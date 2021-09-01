// -*- mode:c++; tab-width:2; indent-tabs-mode:nil; c-basic-offset:2 -*-
/*
 *  Decoder.cpp
 *  zxing
 *
 *  Created by Christian Brunschen on 20/05/2008.
 *  Copyright 2008 ZXing authors All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zxing/qrcode/decoder/Decoder.h>
#include <zxing/qrcode/decoder/BitMatrixParser.h>
#include <zxing/qrcode/ErrorCorrectionLevel.h>
#include <zxing/qrcode/Version.h>
#include <zxing/qrcode/decoder/DataBlock.h>
#include <zxing/qrcode/decoder/DecodedBitStreamParser.h>
#include <zxing/ReaderException.h>
#include <zxing/ChecksumException.h>
#include <zxing/common/reedsolomon/ReedSolomonException.h>

#if(1) //DRP TEST
#include "r_typedefs.h"
#include "perform.h"
#include "r_bcd_main.h"
#include "r_bcd_drp_application.h"
#endif //DRP TEST

using zxing::qrcode::Decoder;
using zxing::DecoderResult;
using zxing::Ref;

// VC++
using zxing::ArrayRef;
using zxing::BitMatrix;

Decoder::Decoder() :
  rsDecoder_(GenericGF::QR_CODE_FIELD_256) {
}

void Decoder::correctErrors(ArrayRef<char> codewordBytes, int numDataCodewords) {
#if(1) //DRP TEST
  int numCodewords = codewordBytes->size();
  int numECCodewords = numCodewords - numDataCodewords;

#ifndef ZXING_CPU_MODE
  bool ret = R_BCD_MainReedsolomon((int8_t *)&codewordBytes[0], numCodewords, numECCodewords);
  if (ret == false) {
//    PerformSetEndTime(10);
    throw ChecksumException();
  }
#else // ZXING_CPU_MODE
  ArrayRef<int> codewordInts(numCodewords);
  for (int i = 0; i < numCodewords; i++) {
    codewordInts[i] = codewordBytes[i] & 0xff;
  }

  try {
    rsDecoder_.decode(codewordInts, numECCodewords);
  } catch (ReedSolomonException const& ignored) {
    (void)ignored;
//    PerformSetEndTime(10);
    throw ChecksumException();
  }

  for (int i = 0; i < numDataCodewords; i++) {
    codewordBytes[i] = (char)codewordInts[i];
  }
#endif // ZXING_CPU_MODE
#endif //DRP TEST
}

Ref<DecoderResult> Decoder::decode(Ref<BitMatrix> bits) {
  // Construct a parser and read version, error-correction level
  BitMatrixParser parser(bits);

  // std::cerr << *bits << std::endl;

  Version *version = parser.readVersion();
  ErrorCorrectionLevel &ecLevel = parser.readFormatInformation()->getErrorCorrectionLevel();


  // Read codewords
  ArrayRef<char> codewords(parser.readCodewords());


  // Separate into data blocks
  std::vector<Ref<DataBlock> > dataBlocks(DataBlock::getDataBlocks(codewords, version, ecLevel));


  // Count total number of data bytes
  int totalBytes = 0;
  for (size_t i = 0; i < dataBlocks.size(); i++) {
    totalBytes += dataBlocks[i]->getNumDataCodewords();
  }
  ArrayRef<char> resultBytes(totalBytes);
  int resultOffset = 0;

#if(1) //DRP TEST
  PerformSetStartTime(TIME_ZXING_REEDSOLOMON_TOTAL);
#endif
  // Error-correct and copy data blocks together into a stream of bytes
  for (size_t j = 0; j < dataBlocks.size(); j++) {
    Ref<DataBlock> dataBlock(dataBlocks[j]);
    ArrayRef<char> codewordBytes = dataBlock->getCodewords();
    int numDataCodewords = dataBlock->getNumDataCodewords();
    correctErrors(codewordBytes, numDataCodewords);
    for (int i = 0; i < numDataCodewords; i++) {
      resultBytes[resultOffset++] = codewordBytes[i];
    }
  }

#if(1) //DRP TEST
  PerformSetEndTime(TIME_ZXING_REEDSOLOMON_TOTAL);
//  R_BCD_MainSetReedsolomonTime(PerformGetElapsedTime_us(TIME_ZXING_REEDSOLOMON_TOTAL));
#endif

  return DecodedBitStreamParser::decode(resultBytes,
                                        version,
                                        ecLevel,
                                        DecodedBitStreamParser::Hashtable());
}

