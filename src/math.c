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

  rx = radians(rx);
  ry = radians(ry);
  rz = radians(rz);

  r32 srx = sinf(rx);
  r32 crx = cosf(rx);
  r32 sry = sinf(ry);
  r32 cry = cosf(ry);
  r32 srz = sinf(rz);
  r32 crz = cosf(rz);
  
  result.m[0] = crz*cry;
  result.m[4] = crz*sry*srx - srz*crx;
  result.m[8] = crz*sry*crx + srz*srx;
  result.m[1] = srz*cry;
  result.m[5] = srz*sry*srx + crz*crx;
  result.m[9] = srz*sry*crx - crz*srx;
  result.m[2] = -sry;
  result.m[6] = cry*srx;
  result.m[10] = cry*crx;

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
