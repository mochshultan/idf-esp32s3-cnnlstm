#ifndef FALL_DETECTION_MODEL_H_
#define FALL_DETECTION_MODEL_H_

// Make sure these match your .cc file exactly
extern const unsigned char fall_detection_model[];
extern const unsigned int fall_detection_model_len;

// Add model info for debugging
// Based on your .cc file info
#define MODEL_SIZE_BYTES 70136

// Model architecture info (adjust based on your training)
#define MODEL_INPUT_SHAPE_BATCH 1
#define MODEL_INPUT_SHAPE_SEQUENCE 100
#define MODEL_INPUT_SHAPE_FEATURES 7
#define MODEL_OUTPUT_CLASSES 5

// For verification
static_assert(MODEL_INPUT_SHAPE_SEQUENCE == 100, "Sequence length mismatch");
static_assert(MODEL_INPUT_SHAPE_FEATURES == 7, "Feature count mismatch");
static_assert(MODEL_OUTPUT_CLASSES == 5, "Class count mismatch");

#endif  // FALL_DETECTION_MODEL_H_