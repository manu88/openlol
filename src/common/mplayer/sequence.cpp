#include <cstdio>

#include "sequence.hpp"
#include "sequence_xmi.hpp"

// ----------------------------------------------------------------------------
Sequence::~Sequence() {}

// ----------------------------------------------------------------------------
Sequence *Sequence::load(const char *path) {
  FILE *file = fopen(path, "rb");
  if (!file)
    return nullptr;

  Sequence *seq = load(file);

  fclose(file);
  return seq;
}

// ----------------------------------------------------------------------------
Sequence *Sequence::load(FILE *file, int offset, size_t size) {
  if (!size) {
    fseek(file, 0, SEEK_END);
    if (ftell(file) < 0)
      return nullptr;
    size = ftell(file) - offset;
  }

  fseek(file, offset, SEEK_SET);
  std::vector<uint8_t> data(size);
  if (fread(data.data(), 1, size, file) != size)
    return nullptr;

  return load(data.data(), size);
}

// ----------------------------------------------------------------------------
Sequence *Sequence::load(const uint8_t *data, size_t size) {
  Sequence *seq = nullptr;

  if (SequenceXMI::isValid(data, size)) {
    seq = new SequenceXMI();
  }

  if (seq) {
    seq->read(data, size);
    seq->reset();
  }

  return seq;
}
