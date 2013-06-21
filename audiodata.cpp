/*************************************************************************

  Copyright 2011-2013 Ibrahim Sha'ath

  This file is part of LibKeyFinder.

  LibKeyFinder is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LibKeyFinder is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LibKeyFinder.  If not, see <http://www.gnu.org/licenses/>.

*************************************************************************/

#include "audiodata.h"

namespace KeyFinder {

  AudioData::AudioData(): samples(0), channels(0), frameRate(0) { }

  unsigned int AudioData::getChannels() const {
    return channels;
  }

  void AudioData::setChannels(unsigned int newChannels) {
    if (newChannels < 1) throw Exception("New channel count must be > 0");
    channels = newChannels;
  }

  unsigned int AudioData::getFrameRate() const {
    return frameRate;
  }

  void AudioData::setFrameRate(unsigned int newFrameRate) {
    if (newFrameRate < 1) throw Exception("New frame rate must be > 0");
    frameRate = newFrameRate;
  }

  void AudioData::append(const AudioData& that) {
    if (channels == 0 && frameRate == 0) {
      channels = that.channels;
      frameRate = that.frameRate;
    }
    if (that.channels != channels)
      throw Exception("Cannot append audio data with a different number of channels");
    if (that.frameRate != frameRate)
      throw Exception("Cannot append audio data with a different frame rate");
    samples.insert(samples.end(), that.samples.begin(), that.samples.end());
  }

  void AudioData::prepend(const AudioData& that) {
    if (channels == 0 && frameRate == 0) {
      channels = that.channels;
      frameRate = that.frameRate;
    }
    if (that.channels != channels)
      throw Exception("Cannot prepend audio data with a different number of channels");
    if (that.frameRate != frameRate)
      throw Exception("Cannot prepend audio data with a different frame rate");
    samples.insert(samples.begin(), that.samples.begin(), that.samples.end());
  }

  // get sample by absolute index
  float AudioData::getSample(unsigned int index) const {
    if (index >= getSampleCount()) {
      std::ostringstream ss;
      ss << "Cannot get out-of-bounds sample (" << index << "/" << getSampleCount() << ")";
      throw Exception(ss.str().c_str());
    }
    return samples[index];
  }

  // get sample by frame and channel
  float AudioData::getSampleByFrame(unsigned int frame, unsigned int channel) const {
    if (frame >= getFrameCount()) {
      std::ostringstream ss;
      ss << "Cannot get out-of-bounds frame (" << frame << "/" << getFrameCount() << ")";
      throw Exception(ss.str().c_str());
    }
    if (channel >= channels) {
      std::ostringstream ss;
      ss << "Cannot get out-of-bounds channel (" << channel << "/" << channels << ")";
      throw Exception(ss.str().c_str());
    }
    return getSample(frame * channels + channel);
  }

  // set sample by absolute index
  void AudioData::setSample(unsigned int index, float value) {
    if (index >= getSampleCount()) {
      std::ostringstream ss;
      ss << "Cannot set out-of-bounds sample (" << index << "/" << getSampleCount() << ")";
      throw Exception(ss.str().c_str());
    }
    if (!boost::math::isfinite(value)) {
      throw Exception("Cannot set sample to NaN");
    }
    samples[index] = value;
  }

  // set sample by frame and channel
  void AudioData::setSampleByFrame(unsigned int frame, unsigned int channel, float value) {
    if (frame >= getFrameCount()) {
      std::ostringstream ss;
      ss << "Cannot set out-of-bounds frame (" << frame << "/" << getFrameCount() << ")";
      throw Exception(ss.str().c_str());
    }
    if (channel >= channels) {
      std::ostringstream ss;
      ss << "Cannot set out-of-bounds channel (" << channel << "/" << channels << ")";
      throw Exception(ss.str().c_str());
    }
    setSample(frame * channels + channel, value);
  }

  void AudioData::addToSampleCount(unsigned int newSamples) {
    samples.resize(getSampleCount() + newSamples, 0.0);
  }

  void AudioData::addToFrameCount(unsigned int newFrames) {
    if (channels < 1) throw Exception("Channels must be > 0");
    addToSampleCount(newFrames * channels);
  }

  unsigned int AudioData::getSampleCount() const {
    return samples.size();
  }

  unsigned int AudioData::getFrameCount() const {
    if (channels < 1) throw Exception("Channels must be > 0");
    return getSampleCount() / channels;
  }

  void AudioData::reduceToMono() {
    if (channels < 2) return;
    std::deque<float>::const_iterator readAt = samples.begin();
    std::deque<float>::iterator writeAt = samples.begin();
    while (readAt < samples.end()) {
      float sum = 0.0;
      for (unsigned int c = 0; c < channels; c++) {
        sum += *readAt;
        std::advance(readAt, 1);
      }
      *writeAt = sum / channels;
      std::advance(writeAt, 1);
    }
    samples.resize(getSampleCount() / channels);
    channels = 1;
  }

  // Strictly to be applied AFTER low pass filtering
  void AudioData::downsample(unsigned int factor, bool shortcut) {
    if (factor == 1) return;
    if (channels > 1) throw Exception("Apply to monophonic only");
    std::deque<float>::const_iterator readAt = samples.begin();
    std::deque<float>::iterator writeAt = samples.begin();
    while (readAt < samples.end()) {
      float mean = 0.0;
      if (shortcut) {
        mean = *readAt;
        std::advance(readAt, factor);
      } else {
        for (unsigned int s = 0; s < factor; s++) {
          if (readAt < samples.end()) {
            mean += *readAt;
            std::advance(readAt, 1);
          }
          mean /= (float)factor;
        }
      }
      *writeAt = mean;
      std::advance(writeAt, 1);
    }
    samples.resize(ceil((float)getSampleCount() / (float)factor));
    setFrameRate(getFrameRate() / factor);
  }

  void AudioData::discardFramesFromFront(unsigned int discardFrameCount) {
    if (discardFrameCount > getFrameCount()) {
      std::ostringstream ss;
      ss << "Cannot discard " << discardFrameCount << " frames of " << getFrameCount();
      throw Exception(ss.str().c_str());
    }
    unsigned int discardSampleCount = discardFrameCount * channels;
    std::deque<float>::iterator discardToHere = samples.begin();
    std::advance(discardToHere, discardSampleCount);
    samples.erase(samples.begin(), discardToHere);
  }

  AudioData* AudioData::sliceSamplesFromBack(unsigned int sliceSampleCount) {
    if (sliceSampleCount > getSampleCount()) {
      std::ostringstream ss;
      ss << "Cannot slice " << sliceSampleCount << " samples of " << getSampleCount();
      throw Exception(ss.str().c_str());
    }
    AudioData* that = new AudioData();
    that->channels = channels;
    that->setFrameRate(getFrameRate());
    that->addToSampleCount(sliceSampleCount);
    std::deque<float>::const_iterator readAt = samples.begin();
    std::advance(readAt, getSampleCount() - sliceSampleCount);
    std::deque<float>::iterator writeAt = that->samples.begin();
    while (readAt < samples.end()) {
      *writeAt = *readAt;
      std::advance(readAt, 1);
      std::advance(writeAt, 1);
    }
    samples.resize(getSampleCount() - sliceSampleCount);
    return that;
  }

  void AudioData::resetIterators() {
    readIterator = samples.begin();
    writeIterator = samples.begin();
  }

  bool AudioData::readIteratorWithinUpperBound() const {
    return (readIterator < samples.end());
  }

  bool AudioData::writeIteratorWithinUpperBound() const {
    return (writeIterator < samples.end());
  }

  void AudioData::advanceReadIterator(unsigned int by) {
    std::advance(readIterator, by);
  }

  void AudioData::advanceWriteIterator(unsigned int by) {
    std::advance(writeIterator, by);
  }

  float AudioData::getSampleAtReadIterator() const {
    return *readIterator;
  }

  void AudioData::setSampleAtWriteIterator(float value) {
    *writeIterator = value;
  }

}
