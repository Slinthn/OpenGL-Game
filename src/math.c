#define PI_32 3.14159265358979323846f
#define radians(deg) ((deg)*PI_32 / 180.0f)

typedef struct {
  float m[16];
} matrix4;

matrix4 perspectiveMatrix(r32 aspect, r32 fov, r32 farz, r32 nearz) {
  matrix4 result = {0};
  result.m[0] = 1.0f / (aspect*tanf(fov / 2.0f));
  result.m[5] = 1.0f / tanf(fov / 2.0f);
  result.m[10] = -(farz + nearz) / (farz - nearz);
  result.m[11] = -1;
  result.m[14] = -(2*farz*nearz) / (farz - nearz);
  return result;
}

matrix4 orthographicMatrix(r32 left, r32 right, r32 top, r32 bottom, r32 farz, r32 nearz) {
  matrix4 result = {0};
  result.m[0] = 2.0f / (right - left);
  result.m[5] = 2.0f / (top - bottom);
  result.m[10] = -2.0f / (farz - nearz);
  result.m[12] = -(right + left) / (right - left);
  result.m[13] = -(top + bottom) / (top - bottom);
  result.m[14] = -(farz + nearz) / (farz - nearz);
  result.m[15] = 1.0f;
  
  return result;
}

matrix4 transformMatrix(r32 tx, r32 ty, r32 tz, r32 rx, r32 ry, r32 rz, r32 sx, r32 sy, r32 sz) {
  matrix4 result = {0};
  result.m[0] = 1;
  result.m[5] = 1;
  result.m[10] = 1;
  result.m[15] = 1;

  rx *= PI_32 / 180.0f;
  ry *= PI_32 / 180.0f;
  rz *= PI_32 / 180.0f;
  
  result.m[0] = cosf(rz)*cosf(ry);
  result.m[4] = cosf(rz)*sinf(ry)*sinf(rx) - sinf(rz)*cosf(rx);
  result.m[8] = cosf(rz)*sinf(ry)*cosf(rx) + sinf(rz)*sinf(rx);
  result.m[1] = sinf(rz)*cosf(ry);
  result.m[5] = sinf(rz)*sinf(ry)*sinf(rx) + cosf(rz)*cosf(rx);
  result.m[9] = sinf(rz)*sinf(ry)*cosf(rx) - cosf(rz)*sinf(rx);
  result.m[2] = -sinf(ry);
  result.m[6] = cosf(ry)*sinf(rx);
  result.m[10] = cosf(ry)*cosf(rx);

  result.m[12] = tx;
  result.m[13] = ty;
  result.m[14] = tz;

  // TODO(slin): does this scale work
  result.m[0] *= sx;
  result.m[1] *= sx;
  result.m[2] *= sx;
  result.m[3] *= sx;

  result.m[4] *= sy;
  result.m[5] *= sy;
  result.m[6] *= sy;
  result.m[7] *= sy;

  result.m[8] *= sz;
  result.m[9] *= sz;
  result.m[10] *= sz;
  result.m[11] *= sz;
  return result;
}
