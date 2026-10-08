#pragma once

#include <IOKit/IOTypes.h>
#include "Ath10kCE.hpp"
#include "Ath10kDMA.hpp"

enum class Ath10kCERingDirection : UInt8 {
    Source = 0,
    Destination = 1,
};

class Ath10kCERing
{
public:
    bool allocate(UInt32 ceId, Ath10kCERingDirection direction, UInt32 entries,
                  IOMapper *mapper)
    {
        release();

        if (ceId >= Ath10k::kCECount || !Ath10k::isPowerOfTwo(entries))
            return false;

        const UInt64 bytes = static_cast<UInt64>(entries) * Ath10k::kCEDescriptorSize;
        if (bytes == 0 || bytes > 0xffffffffULL)
            return false;

        if (!_dma.allocate(bytes, Ath10k::kCEDescriptorRingAlign, mapper))
            return false;

        _ceId = ceId;
        _direction = direction;
        _entries = entries;
        _bytes = bytes;
        return true;
    }

    void release()
    {
        _dma.release();
        _ceId = 0;
        _direction = Ath10kCERingDirection::Source;
        _entries = 0;
        _bytes = 0;
    }

    bool allocated() const { return _dma.allocated() && _entries != 0; }
    UInt32 ceId() const { return _ceId; }
    Ath10kCERingDirection direction() const { return _direction; }
    UInt32 entries() const { return _entries; }
    UInt64 bytes() const { return _bytes; }
    UInt32 dmaAddress32() const { return static_cast<UInt32>(_dma.dmaAddress()); }
    UInt64 dmaAddress() const { return _dma.dmaAddress(); }
    IOPhysicalAddress physicalAddress() const { return _dma.physicalAddress(); }
    bool usesExplicitMapper() const { return _dma.usesExplicitMapper(); }
    const char *mapperModeName() const { return _dma.mapperModeName(); }
    void *virtualAddress() const { return _dma.virtualAddress(); }
    bool syncForDevice() { return _dma.syncForDevice(); }
    bool syncFromDevice() { return _dma.syncFromDevice(); }
    UInt32 syncOutCount() const { return _dma.syncOutCount(); }
    UInt32 syncInCount() const { return _dma.syncInCount(); }

    const char *directionName() const
    {
        return _direction == Ath10kCERingDirection::Source ? "src" : "dst";
    }

private:
    Ath10kDMABuffer _dma;
    UInt32 _ceId = 0;
    Ath10kCERingDirection _direction = Ath10kCERingDirection::Source;
    UInt32 _entries = 0;
    UInt64 _bytes = 0;
};
