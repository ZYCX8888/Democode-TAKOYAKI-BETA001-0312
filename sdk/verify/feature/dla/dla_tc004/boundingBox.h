/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#ifndef _BBOX_H_
#define _BBOX_H_
const float NO_VALUE =  -888.0f;

class NormalizedBBox {
 public:
  explicit NormalizedBBox()
    : xmin_(NO_VALUE), ymin_(NO_VALUE),
      xmax_(NO_VALUE), ymax_(NO_VALUE),
      size_(NO_VALUE) {}

  virtual ~NormalizedBBox(){}

   // optional float xmin = 1;
  inline bool has_xmin() const {
    return (xmin_ != NO_VALUE);
  }
  inline float xmin() const {
    return xmin_;
  }
  inline void set_xmin(float value) {
    xmin_ = value;
  }

  // optional float ymin = 2;
  inline bool has_ymin() const {
    return (ymin_ != NO_VALUE);
  }
  inline float ymin() const {
    return ymin_;
  }
  inline void set_ymin(float value) {
    ymin_ = value;
  }

  // optional float xmax = 3;
  inline bool has_xmax() const {
    return (xmax_ != NO_VALUE);
  }
  inline float xmax() const {
    return xmax_;
  }
  inline void set_xmax(float value) {
    xmax_ = value;
  }

  // optional float ymax = 4;
  inline bool has_ymax() const {
    return (ymax_ != NO_VALUE);
  }
  inline float ymax() const {
    return ymax_;
  }
  inline void set_ymax(float value) {
    ymax_ = value;
  }

  // optional float size;
  inline bool has_size() const {
    return (size_ != NO_VALUE);
  }
  inline float size() const {
    return size_;
  }
  inline void set_size(float value) {
    size_ = value;
  }


 private:
  float xmin_;
  float ymin_;
  float xmax_;
  float ymax_;
  float size_;
};
#endif
