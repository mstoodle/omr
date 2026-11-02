/*******************************************************************************
 * Copyright (c) 2022, 2022 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "gtest/gtest.h"
#include <cstdio>
#include <cstring>
#include "AllocatorRaw.hpp"
#include "JBIRBufferReader.hpp"
#include "JBIRBufferWriter.hpp"
#include "JBIRFileHeader.hpp"
#include "JBIRFileReader.hpp"
#include "JBIRFileWriter.hpp"
#include "JBIRReader.hpp"
#include "JBIRWriter.hpp"

using namespace OMR::JB2;

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Write h to a tmpfile, rewind, and return the FILE*.
// Caller must fclose() it.  Returns nullptr if any step fails.
static FILE *
writeAndRewind(const JBIRFileHeader &h) {
    FILE *f = tmpfile();
    if (f == nullptr)
        return nullptr;
    JBIRFileWriter w(f);
    if (!w.writeHeader(h)) {
        fclose(f);
        return nullptr;
    }
    rewind(f);
    return f;
}

// Write h, corrupt byte at offset `byteOffset` on disk with value `badByte`,
// rewind, attempt a readHeader, and return the read result (expect false).
static bool
corruptByteAndRead(const JBIRFileHeader &h, long byteOffset, uint8_t badByte) {
    FILE *f = tmpfile();
    if (f == nullptr)
        return true; // unexpected failure — let ASSERT handle it upstream
    JBIRFileWriter w(f);
    if (!w.writeHeader(h)) {
        fclose(f);
        return true;
    }
    fseek(f, byteOffset, SEEK_SET);
    fwrite(&badByte, 1, 1, f);
    rewind(f);
    JBIRFileHeader out;
    JBIRFileReader r(f);
    bool result = r.readHeader(out);
    fclose(f);
    return result;
}

// ---------------------------------------------------------------------------
// JBIRFileHeader::current()
// ---------------------------------------------------------------------------

TEST(JBIRFileHeaderTest, CurrentHasCorrectMagic) {
    JBIRFileHeader h = JBIRFileHeader::current();
    EXPECT_EQ(h.magic[0], 'J');
    EXPECT_EQ(h.magic[1], 'B');
    EXPECT_EQ(h.magic[2], 'I');
    EXPECT_EQ(h.magic[3], 'R');
}

TEST(JBIRFileHeaderTest, CurrentHasCorrectVersion) {
    JBIRFileHeader h = JBIRFileHeader::current();
    EXPECT_EQ(h.versionMajor, JBIR_VERSION_MAJOR);
    EXPECT_EQ(h.versionMinor, JBIR_VERSION_MINOR);
    EXPECT_EQ(h.versionPatch, JBIR_VERSION_PATCH);
}

TEST(JBIRFileHeaderTest, CurrentFlags1IsZero) {
    JBIRFileHeader h = JBIRFileHeader::current();
    EXPECT_EQ(h.flags1, 0u);
}

TEST(JBIRFileHeaderTest, CurrentFlags0ReservedBitsAreZero) {
    JBIRFileHeader h = JBIRFileHeader::current();
    EXPECT_EQ(h.flags0 & ~JBIR_FLAG0_LITTLE_ENDIAN, 0u);
}

// ---------------------------------------------------------------------------
// Round-trip: write then read back the native-endian header via File
// ---------------------------------------------------------------------------

TEST(JBIRFileHeaderTest, RoundTripCurrentHeader) {
    JBIRFileHeader written = JBIRFileHeader::current();
    FILE *f = writeAndRewind(written);
    ASSERT_NE(f, nullptr);

    JBIRFileHeader read;
    JBIRFileReader r(f);
    EXPECT_TRUE(r.readHeader(read));
    fclose(f);

    EXPECT_EQ(read.magic[0],     written.magic[0]);
    EXPECT_EQ(read.magic[1],     written.magic[1]);
    EXPECT_EQ(read.magic[2],     written.magic[2]);
    EXPECT_EQ(read.magic[3],     written.magic[3]);
    EXPECT_EQ(read.flags0,       written.flags0);
    EXPECT_EQ(read.flags1,       written.flags1);
    EXPECT_EQ(read.versionMajor, written.versionMajor);
    EXPECT_EQ(read.versionMinor, written.versionMinor);
    EXPECT_EQ(read.versionPatch, written.versionPatch);
}

// ---------------------------------------------------------------------------
// The header must be exactly JBIR_HEADER_SIZE bytes on disk
// ---------------------------------------------------------------------------

TEST(JBIRFileHeaderTest, HeaderIsCorrectSize) {
    JBIRFileHeader h = JBIRFileHeader::current();
    FILE *f = tmpfile();
    ASSERT_NE(f, nullptr);
    JBIRFileWriter w(f);
    ASSERT_TRUE(w.writeHeader(h));
    long pos = ftell(f);
    fclose(f);
    EXPECT_EQ(static_cast<uint32_t>(pos), JBIR_HEADER_SIZE);
}

// ---------------------------------------------------------------------------
// Validation: each magic byte is individually checked
//
// For each of the 4 magic byte positions we write a valid header, corrupt
// only that one byte to a value that is not the correct magic byte, and
// confirm the reader rejects it.  The corruption value 'X' is chosen to
// differ from every valid magic character ('J','B','I','R').
// ---------------------------------------------------------------------------

TEST(JBIRFileHeaderTest, BadMagicByte0Rejected) {
    JBIRFileHeader h = JBIRFileHeader::current();
    EXPECT_FALSE(corruptByteAndRead(h, 0, 'X'))
        << "magic byte 0 corruption not detected";
}

TEST(JBIRFileHeaderTest, BadMagicByte1Rejected) {
    JBIRFileHeader h = JBIRFileHeader::current();
    EXPECT_FALSE(corruptByteAndRead(h, 1, 'X'))
        << "magic byte 1 corruption not detected";
}

TEST(JBIRFileHeaderTest, BadMagicByte2Rejected) {
    JBIRFileHeader h = JBIRFileHeader::current();
    EXPECT_FALSE(corruptByteAndRead(h, 2, 'X'))
        << "magic byte 2 corruption not detected";
}

TEST(JBIRFileHeaderTest, BadMagicByte3Rejected) {
    JBIRFileHeader h = JBIRFileHeader::current();
    EXPECT_FALSE(corruptByteAndRead(h, 3, 'X'))
        << "magic byte 3 corruption not detected";
}

// ---------------------------------------------------------------------------
// Validation: non-zero reserved bits in flags0 are rejected
// ---------------------------------------------------------------------------

TEST(JBIRFileHeaderTest, Flags0ReservedBitsRejected) {
    JBIRFileHeader h = JBIRFileHeader::current();
    h.flags0 |= 0x02u;  // set a reserved bit
    FILE *f = writeAndRewind(h);
    ASSERT_NE(f, nullptr);
    JBIRFileHeader out;
    JBIRFileReader r(f);
    EXPECT_FALSE(r.readHeader(out));
    fclose(f);
}

// ---------------------------------------------------------------------------
// Validation: non-zero flags1 is rejected
// ---------------------------------------------------------------------------

TEST(JBIRFileHeaderTest, Flags1NonZeroRejected) {
    JBIRFileHeader h = JBIRFileHeader::current();
    h.flags1 = 0x01u;
    FILE *f = writeAndRewind(h);
    ASSERT_NE(f, nullptr);
    JBIRFileHeader out;
    JBIRFileReader r(f);
    EXPECT_FALSE(r.readHeader(out));
    fclose(f);
}

// ---------------------------------------------------------------------------
// Validation: truncated file (only magic bytes, no flags or version) rejected
// ---------------------------------------------------------------------------

TEST(JBIRFileHeaderTest, TruncatedHeaderRejected) {
    FILE *f = tmpfile();
    ASSERT_NE(f, nullptr);
    const uint8_t magic[4] = { 'J', 'B', 'I', 'R' };
    fwrite(magic, 1, 4, f);
    rewind(f);
    JBIRFileHeader out;
    JBIRFileReader r(f);
    EXPECT_FALSE(r.readHeader(out));
    fclose(f);
}

// ---------------------------------------------------------------------------
// Buffer writer and reader tests
// ---------------------------------------------------------------------------

TEST(JBIRBufferTest, RoundTripHeaderAndValues) {
    AllocatorRaw alloc;
    JBIRBufferWriter w(&alloc, 16);

    JBIRFileHeader written = JBIRFileHeader::current();
    ASSERT_TRUE(w.writeHeader(written));
    ASSERT_TRUE(w.writeUInt16(0x1234));
    ASSERT_TRUE(w.writeUInt32(0x56789abc));
    ASSERT_TRUE(w.writeUInt64(0x0123456789abcdefULL));

    EXPECT_EQ(w.bytesWritten(), JBIR_HEADER_SIZE + 2 + 4 + 8);

    JBIRBufferReader r(w);
    JBIRFileHeader readH;
    ASSERT_TRUE(r.readHeader(readH));
    EXPECT_EQ(readH.magic[0], written.magic[0]);
    EXPECT_EQ(readH.versionMajor, written.versionMajor);

    uint16_t v16 = 0;
    uint32_t v32 = 0;
    uint64_t v64 = 0;
    ASSERT_TRUE(r.readUInt16(v16));
    ASSERT_TRUE(r.readUInt32(v32));
    ASSERT_TRUE(r.readUInt64(v64));
    EXPECT_EQ(v16, 0x1234);
    EXPECT_EQ(v32, 0x56789abc);
    EXPECT_EQ(v64, 0x0123456789abcdefULL);
    EXPECT_EQ(r.remaining(), 0u);

    // Reading past the end returns false
    uint16_t extra = 0;
    EXPECT_FALSE(r.readUInt16(extra));
}

TEST(JBIRBufferTest, DynamicGrowthExceedingInitialCapacity) {
    AllocatorRaw alloc;
    // Start with a small capacity (e.g. 4 bytes)
    JBIRBufferWriter w(&alloc, 4);
    EXPECT_EQ(w.capacity(), 4u);

    JBIRFileHeader written = JBIRFileHeader::current();
    ASSERT_TRUE(w.writeHeader(written));
    // Header is 12 bytes, so capacity should have grown
    EXPECT_GE(w.capacity(), JBIR_HEADER_SIZE);
    EXPECT_EQ(w.bytesWritten(), JBIR_HEADER_SIZE);

    // Write 100 uint32 values to force several reallocations
    for (uint32_t i = 0; i < 100; ++i) {
        ASSERT_TRUE(w.writeUInt32(i));
    }
    EXPECT_EQ(w.bytesWritten(), JBIR_HEADER_SIZE + 100 * sizeof(uint32_t));

    JBIRBufferReader r(w.buffer(), w.bytesWritten());
    JBIRFileHeader readH;
    ASSERT_TRUE(r.readHeader(readH));
    EXPECT_EQ(readH.versionMajor, written.versionMajor);

    for (uint32_t i = 0; i < 100; ++i) {
        uint32_t val = 0;
        ASSERT_TRUE(r.readUInt32(val));
        EXPECT_EQ(val, i);
    }
    EXPECT_EQ(r.remaining(), 0u);
}
