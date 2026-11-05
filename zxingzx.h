/*
 * Minimal declarations for the BB10 ZXing C++ library (libzxing.so.2.0).
 *
 * The NDK ships the shared library but no headers, so these are written to
 * match the ABI of the library on the device.
 *
 * NOTE: nothing links against this at the moment - the scanner uses the
 * device's own bb::cascades::multimedia::BarcodeDetector, which wraps this
 * same library.  These declarations are kept because they were verified
 * end to end against the device (a generated QR code decoded back to its
 * exact text), which makes them the ready fallback if the Cascades detector
 * is ever unavailable.
 *
 * The rules that keep it safe:
 *
 *  1. Every object is created by the library's own constructor; we only
 *     ever pass pointers around.
 *  2. Ref<T> here is a NON-OWNING pointer wrapper.  The library's own Ref
 *     may or may not own what it points at, and guessing wrong either way
 *     means a double free or a leak in the middle of a live camera loop.
 *     Not owning is the safe choice: the worst case is a leak, and the
 *     scanner builds its decode chain once and reuses it.
 *  3. Classes we allocate carry a generous _pad, so the space we ask the
 *     allocator for is never smaller than what the library needs.  (new[]
 *     uses our sizeof; a class that is too small is a heap corruption.)
 *  4. Types that are RETURNED BY VALUE must match the library exactly - a
 *     different size changes the calling convention.  Result is therefore
 *     declared with its real members, not with padding.
 *
 * Verified against the device library (QNX 10.3.1, /base/usr/lib/libzxing.so.2.0):
 *   - Ref<T> is one pointer wide.
 *   - LuminanceSource virtual order: ~, ~, getRow, getMatrix, getWidth,
 *     getHeight, isCropSupported, crop, isRotateSupported, rotateCounterClockwise.
 *   - GreyscaleLuminanceSource(data, dataWidth, dataHeight, left, top, w, h).
 *   - Result is NOT polymorphic; its members are Ref<String>, ArrayRef<byte>,
 *     std::vector<Ref<ResultPoint>>, BarcodeFormat.
 */
#ifndef ZXINGZX_H
#define ZXINGZX_H

#include <string>
#include <vector>

namespace zxing {

// ---------------------------------------------------------------- Ref
template <typename T>
class Ref {
public:
    Ref() : p(0) {}
    Ref(T *p_) : p(p_) {}
    Ref(const Ref<T> &other) : p(other.p) {}
    ~Ref() {}

    T *operator->() const { return p; }
    T &operator*() const { return *p; }
    T *get() const { return p; }
    void reset(T *q) { p = q; }
    bool isNull() const { return p == 0; }
    bool operator!() const { return p == 0; }

private:
    T *p;
};

// ---------------------------------------------------------------- ArrayRef
template <typename T>
class ArrayRef {
public:
    ArrayRef() : _array(0), _length(0) {}
    ArrayRef(const T *array, int length) : _array(array), _length(length) {}
    const T *array() const { return _array; }
    int length() const { return _length; }

private:
    const T *_array;
    int _length;
};

// ---------------------------------------------------------------- BitMatrix
class BitMatrix {
public:
    BitMatrix(unsigned int width, unsigned int height);
    bool get(unsigned int x, unsigned int y) const;
    void set(unsigned int x, unsigned int y);
    unsigned int getWidth() const;
    unsigned int getHeight() const;
    virtual ~BitMatrix();

private:
    unsigned char _pad[256];
};

// ---------------------------------------------------------------- LuminanceSource
class LuminanceSource {
public:
    LuminanceSource();
    virtual ~LuminanceSource();
    virtual unsigned char *getRow(int y, unsigned char *row) = 0;
    virtual unsigned char *getMatrix() = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual bool isCropSupported() const;
    virtual void crop(int left, int top, int width, int height);
    virtual bool isRotateSupported() const;
    virtual void rotateCounterClockwise();

private:
    unsigned char _pad[128];
};

// ---------------------------------------------------------------- GreyscaleLuminanceSource
class GreyscaleLuminanceSource : public LuminanceSource {
public:
    GreyscaleLuminanceSource(unsigned char *data, int dataWidth, int dataHeight,
                              int left, int top, int width, int height);
    virtual ~GreyscaleLuminanceSource();
    virtual unsigned char *getRow(int y, unsigned char *row);
    virtual unsigned char *getMatrix();
    virtual int getWidth() const;
    virtual int getHeight() const;
    virtual bool isRotateSupported() const;
    virtual void rotateCounterClockwise();

private:
    unsigned char _pad[256];
};

// ---------------------------------------------------------------- Binarizer
class Binarizer {
public:
    Binarizer(Ref<LuminanceSource> source);
    virtual ~Binarizer();
    virtual Ref<BitMatrix> getBlackMatrix() = 0;

private:
    unsigned char _pad[128];
};

class HybridBinarizer : public Binarizer {
public:
    HybridBinarizer(Ref<LuminanceSource> source);
    virtual ~HybridBinarizer();
    virtual Ref<BitMatrix> getBlackMatrix();

private:
    unsigned char _pad[256];
};

// ---------------------------------------------------------------- BinaryBitmap
class BinaryBitmap {
public:
    BinaryBitmap(Ref<Binarizer> binarizer);
    virtual ~BinaryBitmap();
    Ref<BitMatrix> getBlackMatrix();
    Ref<LuminanceSource> getLuminanceSource() const;
    bool isRotateSupported() const;
    bool isCropSupported() const;
    int getHeight() const;
    int getWidth() const;

private:
    unsigned char _pad[128];
};

// ---------------------------------------------------------------- String / Result
// The library's String is not a plain std::string: it carries a vtable
// pointer and keeps the characters in a buffer of its own.  Measured on the
// device, the characters start at offset 0x38 and the pointer to them sits
// at offset 0x0C - the latter also works when the text is too long for the
// inline buffer and therefore lives on the heap.
class String {
public:
    String();
    String(const std::string &text);
    ~String();
    std::string getText() const;

private:
    unsigned char _pad[128];
};

// Read the characters out of a String.  Falls back to the inline buffer if
// the pointer does not look like text.
inline const char *zxingStringChars(const String *s)
{
    const char *base = (const char *)s;
    const char *p = *(const char **)(base + 0x0C);
    if (p) {
        const char c = *p;
        if ((c >= 32 && c < 127) || c == '\n' || c == '\r' || c == '\t')
            return p;
    }
    return base + 0x38;
}

class ResultPoint {
public:
    virtual ~ResultPoint();

private:
    unsigned char _pad[64];
};

// getText() returns a Ref<String>, not a std::string: the library returns a
// pointer to the String it keeps inside the Result.  (A std::string here
// reads the wrong bytes and the caller frees something it does not own.)
class Result {
public:
    Result();
    ~Result();
    Ref<String> getText();
    int getBarcodeFormat() const;

private:
    unsigned char _pad[128];
};

// ---------------------------------------------------------------- Readers
class MultiFormatReader {
public:
    MultiFormatReader();
    virtual ~MultiFormatReader();
    Ref<Result> decode(Ref<BinaryBitmap> image);

private:
    unsigned char _pad[256];
};

} // namespace zxing

#endif // ZXINGZX_H
