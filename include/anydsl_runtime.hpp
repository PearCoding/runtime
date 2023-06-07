#ifndef ANYDSL_RUNTIME_HPP
#define ANYDSL_RUNTIME_HPP

#include "anydsl_runtime.h"

#ifndef ANYDSL_NAMESPACE
#define ANYDSL_NAMESPACE anydsl
#endif

namespace ANYDSL_NAMESPACE {

enum class Platform : int32_t {
    Host   = AnyDSL_DEVICE_HOST,
    Cuda   = AnyDSL_DEVICE_CUDA,
    OpenCL = AnyDSL_DEVICE_OPENCL,
    HSA    = AnyDSL_DEVICE_HSA
};

class Device {
public:
    Device(Platform platform = Platform::Host, int32_t num = 0)
        : mNum(num)
    {
        AnyDSLGetDeviceRequest info = {
            AnyDSL_STRUCTURE_TYPE_GET_DEVICE_REQUEST,
            nullptr,
            (AnyDSLDeviceType)platform,
            num
        };

        anydslGetDevice(&info, &mHandle);
    }

    inline AnyDSLDevice handle() const { return mHandle; }

protected:
    int32_t mNum;
    AnyDSLDevice mHandle;
};

template <typename T>
class Array {
public:
    Array()
        : mData(nullptr)
        , mSize(0)
        , mDevice()
    {
    }

    Array(int64_t size)
        : Array(Device(), size)
    {
    }

    Array(const Device& dev, int64_t size)
        : mDevice(dev)
    {
        allocate(size);
    }

    Array(Array&& other)
        : mData(other.mData)
        , mSize(other.mSize)
        , mDevice(other.mDevice)
    {
        other.mData = nullptr;
    }

    Array& operator=(Array&& other)
    {
        deallocate();
        mDevice     = other.mDevice;
        mSize       = other.mSize;
        mData       = other.mData;
        other.data_ = nullptr;
        return *this;
    }

    Array(const Array&)            = delete;
    Array& operator=(const Array&) = delete;

    ~Array() { deallocate(); }

    T* begin() { return mData; }
    const T* begin() const { return mData; }

    T* end() { return mData + mSize; }
    const T* end() const { return mData + mSize; }

    T* data() { return mData; }
    const T* data() const { return mData; }

    int64_t size() const { return mSize; }
    const Device& device() const { return mDevice; }

    const T& operator[](int i) const { return mData[i]; }
    T& operator[](int i) { return mData[i]; }

    T* release()
    {
        T* ptr = mData;
        mData  = nullptr;
        mSize  = 0;
        return ptr;
    }

    inline AnyDSLBuffer handle() const { return mBuffer; }

protected:
    void allocate(int64_t size)
    {
        if (!mData)
            deallocate();

        mSize = size;

        AnyDSLCreateBufferInfo info = {
            AnyDSL_STRUCTURE_TYPE_CREATE_BUFFER_INFO,
            nullptr,
            sizeof(T) * size,
            0
        };
        anydslCreateBuffer(mDevice.handle(), &info, &mBuffer); // TODO: Check return value

        AnyDSLGetBufferPointerInfo ptrInfo = {
            AnyDSL_STRUCTURE_TYPE_GET_BUFFER_POINTER_INFO,
            nullptr,
            0                                      // Will be set by the function
        };
        anydslGetBufferPointer(mBuffer, &ptrInfo); // TODO: Check return value

        mData = reinterpret_cast<T*>(ptrInfo.pointer);
    }

    void deallocate()
    {
        if (mData) {
            anydslDestroyBuffer(mBuffer);
            mData = nullptr;
        }
    }

    T* mData;
    int64_t mSize;
    Device mDevice;
    AnyDSLBuffer mBuffer;
};

template <typename T>
void copy(const Array<T>& a, int64_t offset_a, Array<T>& b, int64_t offset_b, int64_t size)
{
    AnyDSLBufferCopy region = {
        offset_a, offset_b, a.size() * sizeof(T)
    };
    anydslCopyBuffer(a.handle(), b.handle(), 1, &region);
}

template <typename T>
void copy(const Array<T>& a, Array<T>& b, int64_t size)
{
    copy(a, 0, b, 0, size);
}

template <typename T>
void copy(const Array<T>& a, Array<T>& b)
{
    copy(a, b, a.size());
}

} // namespace AnyDSL

#endif // ANYDSL_RUNTIME_HPP