#include "mem.h"

#include <lion/lion.h>
#include <lion_sim/files.h>
#include <lion_utils/macros.h>
#include <lion_utils/vendor/log.h>
#include <stdio.h>
#include <string.h>

lion_status_t lion_vector_new(lion_sim_t *sim, const size_t data_size, lion_vector_t *out) {
  lion_vector_t result = {
    .data      = NULL,
    .data_size = data_size,
    .len       = 0,
    .capacity  = 0,
  };
  *out = result;
  return LION_STATUS_SUCCESS;
}

lion_status_t lion_vector_zero(lion_sim_t *sim, const size_t len, const size_t data_size, lion_vector_t *out) {
  void *data = lion_calloc(sim, len, data_size);
  if (data == NULL) {
    logi_error("Could not allocate enough data");
    return LION_STATUS_FAILURE;
  }

  lion_vector_t result = {
    .data      = data,
    .data_size = data_size,
    .len       = len,
    .capacity  = len,
  };
  *out = result;
  return LION_STATUS_SUCCESS;
}

lion_status_t lion_vector_with_capacity(lion_sim_t *sim, const size_t capacity, const size_t data_size, lion_vector_t *out) {
  void *data = lion_malloc(sim, data_size * capacity);
  if (data == NULL) {
    logi_error("Could not allocate enough data");
    return LION_STATUS_FAILURE;
  }

  lion_vector_t result = {
    .data      = data,
    .data_size = data_size,
    .len       = 0,
    .capacity  = capacity,
  };
  *out = result;
  return LION_STATUS_SUCCESS;
}

lion_status_t lion_vector_from_array(lion_sim_t *sim, const void *data, const size_t len, const size_t data_size, lion_vector_t *out) {
  void *new_data = lion_malloc(sim, len * data_size);
  if (new_data == NULL) {
    logi_error("Could not allocate enough data");
    return LION_STATUS_FAILURE;
  }

  memcpy(new_data, data, len * data_size);
  lion_vector_t result = {
    .data      = new_data,
    .data_size = data_size,
    .len       = len,
    .capacity  = len,
  };
  *out = result;
  return LION_STATUS_SUCCESS;
}

lion_status_t lion_vector_from_csv(lion_sim_t *sim, const char *filename, const size_t data_size, const char *format, lion_vector_t *out) {
  logi_warn("This function assumes only one column with a header");

  logi_debug("Opening file '%s'", filename);
  logi_debug("Using format '%s'", format);
  FILE *f = fopen(filename, "r");
  if (f == NULL) {
    logi_error("Could not open file '%s'", filename);
    return LION_STATUS_FAILURE;
  }

  logi_debug("Allocating line buffer");
  char *line_buffer = lion_malloc(sim, sizeof(char) * 128);
  if (line_buffer == NULL) {
    logi_error("Could not allocate line buffer");
    fclose(f);
    return LION_STATUS_FAILURE;
  }
  logi_debug("Reading number of lines");
  int lines = lion_count_lines(f) - 1;
  logi_debug("Input file has %i lines", lines);
  if (fseek(f, 0, SEEK_SET) != 0) {
    logi_error("Error seeking beginning, found errno %i", errno);
    fclose(f);
    return LION_STATUS_FAILURE;
  }
  logi_debug("Initializing output vector");
  lion_vector_t values;
  LION_CALL_I(lion_vector_with_capacity(sim, lines, sizeof(double), &values), "Could not initialize vector");

  logi_debug("Creating temporary retainers");
  double val;
  char  *buffer;
  logi_debug("Discarding first line");
  LION_VCALL_I(lion_readline(sim, f, line_buffer, &buffer), "Failed reading first line");

  logi_debug("Reading values");
  for (int i = 0; i < lines; i++) {
    LION_VCALL_I(lion_readline(sim, f, line_buffer, &buffer), "Failed reading line %i", i);
    int ret = sscanf(line_buffer, format, &val);
    if (ret != 1) {
      logi_error("Found failure at %i-th value, ret = %i", i, ret);
      fclose(f);
      return LION_STATUS_FAILURE;
    }
    LION_VCALL_I(lion_vector_push(sim, &values, &val), "Failed pushing %i-th value", i);
  }

  logi_debug("Finished reading values");
  *out = values;
  fclose(f);
  lion_free(sim, line_buffer);
  return LION_STATUS_SUCCESS;
}

lion_status_t lion_vector_linspace_d(lion_sim_t *sim, double low, double high, int num, lion_vector_t *out) {
  lion_vector_t vec;
  LION_VCALL_I(lion_vector_with_capacity(sim, num, sizeof(double), &vec), "Failed allocating vector with %d elements", num);
  double step = (high - low) / (double)(num - 1);
  for (int i = 0; i < num; i++) {
    LION_VCALL_I(lion_vector_push_d(sim, &vec, low + i * step), "Failed pushing %d-th element", i);
  }
  *out = vec;
  return LION_STATUS_SUCCESS;
}

lion_status_t lion_vector_linspace_f(lion_sim_t *sim, float low, float high, int num, lion_vector_t *out) {
  lion_vector_t vec;
  LION_VCALL_I(lion_vector_with_capacity(sim, num, sizeof(float), &vec), "Failed allocating vector with %d elements", num);
  float step = (high - low) / (float)(num - 1);
  for (int i = 0; i < num; i++) {
    LION_VCALL_I(lion_vector_push_f(sim, &vec, low + i * step), "Failed pushing %d-th element", i);
  }
  *out = vec;
  return LION_STATUS_SUCCESS;
}

lion_status_t lion_vector_to_csv(lion_sim_t *sim, lion_vector_t *vec, const char *header, const char *filename) {
  // TODO: Implement saving vector to csv file
  logi_error("Saving to csv not currently implemented");
  return LION_STATUS_FAILURE;
}

lion_status_t lion_vector_cleanup(lion_sim_t *sim, const lion_vector_t *const vec) {
  lion_free(sim, vec->data);
  return LION_STATUS_SUCCESS;
}

lion_status_t lion_vector_get(lion_sim_t *sim, const lion_vector_t *vec, const size_t i, void *out) {
  if (vec->data == NULL || i >= vec->len || out == NULL) {
    logi_error("Out of bounds or output is NULL");
    return LION_STATUS_FAILURE;
  }
  memcpy(out, (char *)vec->data + i * vec->data_size, vec->data_size);
  return LION_STATUS_SUCCESS;
}

int8_t lion_vector_get_i8(lion_sim_t *sim, const lion_vector_t *vec, const size_t i) { return *(int8_t *)((char *)vec->data + i * vec->data_size); }

int16_t lion_vector_get_i16(lion_sim_t *sim, const lion_vector_t *vec, const size_t i) {
  return *(int16_t *)((char *)vec->data + i * vec->data_size);
}

int32_t lion_vector_get_i32(lion_sim_t *sim, const lion_vector_t *vec, const size_t i) {
  return *(int32_t *)((char *)vec->data + i * vec->data_size);
}

int64_t lion_vector_get_i64(lion_sim_t *sim, const lion_vector_t *vec, const size_t i) {
  return *(int64_t *)((char *)vec->data + i * vec->data_size);
}

uint8_t lion_vector_get_u8(lion_sim_t *sim, const lion_vector_t *vec, const size_t i) { return *(uint8_t *)((char *)vec->data + i * vec->data_size); }

uint16_t lion_vector_get_u16(lion_sim_t *sim, const lion_vector_t *vec, const size_t i) {
  return *(uint16_t *)((char *)vec->data + i * vec->data_size);
}

uint32_t lion_vector_get_u32(lion_sim_t *sim, const lion_vector_t *vec, const size_t i) {
  return *(uint32_t *)((char *)vec->data + i * vec->data_size);
}

uint64_t lion_vector_get_u64(lion_sim_t *sim, const lion_vector_t *vec, const size_t i) {
  return *(uint64_t *)((char *)vec->data + i * vec->data_size);
}

float lion_vector_get_f(lion_sim_t *sim, const lion_vector_t *vec, const size_t i) { return *(float *)((char *)vec->data + i * vec->data_size); }

double lion_vector_get_d(lion_sim_t *sim, const lion_vector_t *vec, const size_t i) { return *(double *)((char *)vec->data + i * vec->data_size); }

void *lion_vector_get_p(lion_sim_t *sim, const lion_vector_t *vec, const size_t i) { return (void *)((char *)vec->data + i * vec->data_size); }

lion_status_t lion_vector_set(lion_sim_t *sim, lion_vector_t *vec, const size_t i, const void *src) {
  if (vec->data == NULL || i >= vec->len || src == NULL) {
    logi_error("Out of bounds or source is NULL");
    return LION_STATUS_FAILURE;
  }
  memcpy((char *)vec->data + i * vec->data_size, src, vec->data_size);
  return LION_STATUS_SUCCESS;
}

lion_status_t lion_vector_resize(lion_sim_t *sim, lion_vector_t *vec, const size_t new_capacity) {
  void *data = lion_realloc(sim, vec->data, new_capacity * vec->data_size);
  if (data == NULL) {
    logi_error("Could not allocate enough data");
    return LION_STATUS_FAILURE;
  }

  if (new_capacity < vec->len) {
    vec->len = new_capacity;
  }
  vec->capacity = new_capacity;
  vec->data     = data;
  return LION_STATUS_SUCCESS;
}

lion_status_t lion_vector_push(lion_sim_t *sim, lion_vector_t *vec, const void *src) {
  if (src == NULL) {
    logi_error("Source is NULL");
    return LION_STATUS_FAILURE;
  }

  if (vec->len == vec->capacity) {
    // The vector is full so we have to reallocate
    // Default strategy is duplicate the current capacity
    // Perhaps the user can customize the behaviour?
    size_t new_size = 2 * vec->capacity * vec->data_size;
    if (new_size == 0) {
      new_size = vec->data_size;
    }
    logi_debug("Reallocating for %d B", new_size);
    void *new_data = lion_realloc(sim, vec->data, new_size);
    if (new_data == NULL) {
      logi_error("Could not allocate enough data");
      return LION_STATUS_FAILURE;
    }
    memcpy((char *)new_data + vec->len * vec->data_size, src, vec->data_size);
    vec->data = new_data;
    vec->len++;
    vec->capacity = new_size / vec->data_size;
  } else {
    // It can be assumed that vec->len < vec->capacity, so this
    // means we dont have to allocate more memory.
    memcpy((char *)vec->data + vec->len * vec->data_size, src, vec->data_size);
    vec->len++;
  }
  return LION_STATUS_SUCCESS;
}

lion_status_t lion_vector_push_d(lion_sim_t *sim, lion_vector_t *vec, double src) { return lion_vector_push(sim, vec, &src); }

lion_status_t lion_vector_push_f(lion_sim_t *sim, lion_vector_t *vec, float src) { return lion_vector_push(sim, vec, &src); }

lion_status_t lion_vector_extend_array(lion_sim_t *sim, lion_vector_t *vec, const void *src, const size_t len) {
  if (src == NULL) {
    logi_error("Source is NULL");
    return LION_STATUS_FAILURE;
  }

  // size_t delta = vec->len + len - vec->capacity;
  int64_t _delta = (int64_t)(vec->len + len - vec->capacity);
  size_t  delta  = 0;
  if (_delta > 0) {
    delta = (size_t)_delta;
  }
  if (delta > 0) {
    // The vector does not have enough space so we have to allocate more
    // memory
    logi_info("Allocating memory for extension of vector");
    logi_debug("Allocating %d more bytes", delta * vec->data_size);
    void *new_data = lion_realloc(sim, vec->data, (vec->capacity + delta) * vec->data_size);
    if (new_data == NULL) {
      logi_error("Reallocation failed");
      return LION_STATUS_FAILURE;
    }
    logi_debug("Copying new memory");
    memcpy((char *)new_data + vec->len * vec->data_size, src, len * vec->data_size);
    vec->data      = new_data;
    vec->len      += len;
    vec->capacity += delta;
    logi_debug("New len is %d", vec->len);
    logi_debug("New capacity is %d", vec->capacity);
  } else {
    // It can be assumed that vec->len < vec->capacity, so this
    // means we dont have to allocate more memory.
    memcpy((char *)vec->data + vec->len * vec->data_size, src, len * vec->data_size);
    vec->len += len;
  }
  return LION_STATUS_SUCCESS;
}

extern inline size_t lion_vector_total_size(lion_sim_t *sim, const lion_vector_t *vec) { return vec->len * vec->data_size; }

extern inline size_t lion_vector_alloc_size(lion_sim_t *sim, const lion_vector_t *vec) { return vec->capacity * vec->data_size; }
