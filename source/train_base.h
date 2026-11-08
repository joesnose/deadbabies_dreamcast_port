#pragma once
#include <algorithm>

// Extend only the train below the standing plane. World geometry above that
// plane is unchanged; include overscan so camera shake cannot expose a gap.
struct TrainBaseStretch {
  static constexpr float standingY=460.f;
  float scale=1.f;
  TrainBaseStretch(float opaqueBottom,float cameraY) {
    if(opaqueBottom>standingY)
      scale=std::max(1.f,(500.f+32.f-cameraY-standingY)/(opaqueBottom-standingY));
  }
  float map(float y) const {
    return y<=standingY?y:standingY+(y-standingY)*scale;
  }
};
