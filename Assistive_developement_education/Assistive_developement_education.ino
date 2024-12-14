#include <PDM.h>
#include <Nexo_voice_recognition_inferencing.h>

/** Audio buffer and inference settings */
#define BUFFER_SIZE 2048
#define KEYWORD "Nexo"
#define CONFIDENCE_THRESHOLD 0.7
#define MIC_GAIN 100 // Adjust sensitivity here (0-127)

static signed short sampleBuffer[BUFFER_SIZE];
static bool debug_nn = false; // Set to true for debug output

const int LED_PIN = 8; // LED pin

typedef struct {
    int16_t *buffer;
    volatile uint32_t buf_count;
    volatile uint8_t buf_ready;
    uint32_t n_samples;
} inference_t;

static inference_t inference;

/** Function prototypes */
bool start_microphone_inference(uint32_t n_samples);
bool record_audio();
int audio_signal_get_data(size_t offset, size_t length, float *out_ptr);
void end_microphone_inference();

void setup() {
    Serial.begin(115200);
    while (!Serial);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.println("Starting Keyword Detection");

    // Print model details
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: %.2f ms.\n", (float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / (EI_CLASSIFIER_FREQUENCY / 1000));
    ei_printf("\tNumber of classes: %d\n",
              sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

    // Start microphone inference
    if (!start_microphone_inference(EI_CLASSIFIER_RAW_SAMPLE_COUNT)) {
        ei_printf("Error initializing microphone inference.\n");
    }
}

void loop() {
    // Listen for keywords
    ei_printf("Listening for '%s'...\n", KEYWORD);

    if (record_audio()) {
        ei_printf("Audio recorded. Processing...\n");

        signal_t signal;
        signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
        signal.get_data = &audio_signal_get_data;

        ei_impulse_result_t result = {0};

        // Run the classifier
        EI_IMPULSE_ERROR classifier_status = run_classifier(&signal, &result, debug_nn);
        if (classifier_status != EI_IMPULSE_OK) {
            ei_printf("Error running classifier (%d)\n", classifier_status);
            return;
        }

        // Check for keyword detection
        for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            if (strcmp(result.classification[i].label, KEYWORD) == 0 &&
                result.classification[i].value >= CONFIDENCE_THRESHOLD) {
                ei_printf("Detected '%s' with confidence %.2f\n", KEYWORD, result.classification[i].value);
                digitalWrite(LED_PIN, HIGH);
                delay(2000); // Keep LED on for 2 seconds
                digitalWrite(LED_PIN, LOW);
            }
        }

#if EI_CLASSIFIER_HAS_ANOMALY == 1
        ei_printf("Anomaly score: %.3f\n", result.anomaly);
#endif
    } else {
        ei_printf("No audio data recorded. Retrying...\n");
    }
}

/**
 * Start microphone inference.
 */
bool start_microphone_inference(uint32_t n_samples) {
    inference.buffer = (int16_t *)malloc(n_samples * sizeof(int16_t));
    if (!inference.buffer) {
        ei_printf("Failed to allocate buffer.\n");
        return false;
    }

    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    PDM.onReceive([]() {
        int bytesAvailable = PDM.available();
        int bytesRead = PDM.read((char *)sampleBuffer, bytesAvailable);

        for (int i = 0; i < bytesRead / 2; i++) {
            inference.buffer[inference.buf_count++] = sampleBuffer[i];
            if (inference.buf_count >= inference.n_samples) {
                inference.buf_ready = 1;
                inference.buf_count = 0; // Reset for next recording
                break;
            }
        }
    });

    PDM.setBufferSize(4096);

    // Set microphone gain for sensitivity
    PDM.setGain(MIC_GAIN);

    if (!PDM.begin(1, EI_CLASSIFIER_FREQUENCY)) {
        ei_printf("Failed to start PDM.\n");
        end_microphone_inference();
        return false;
    }

    return true;
}

/**
 * Record audio.
 */
bool record_audio() {
    inference.buf_ready = 0;
    while (!inference.buf_ready) {
        delay(800);
    }
    return true;
}

/**
 * Get audio data for the signal.
 */
int audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    numpy::int16_to_float(&inference.buffer[offset], out_ptr, length);
    return 0;
}

/**
 * End microphone inference.
 */
void end_microphone_inference() {
    PDM.end();
    if (inference.buffer) {
        free(inference.buffer);
        inference.buffer = nullptr;
    }
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif
