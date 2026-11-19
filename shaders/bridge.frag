#version 330 core
in vec3 N, worldPosition, tint;
in vec4 lightPosition;
out vec4 outputColor;
uniform vec3 color, eye, sunDirection;
uniform int lit, part, hieu, waterRipples, clearanceCutaway, presentation,
    shadows;
uniform float environmentFade, time, cutawayY;
uniform sampler2D shadowMap;
const float PI = 3.14159265;
vec3 linearColor(vec3 c) {
  return mix(c / 12.92, pow((c + .055) / 1.055, vec3(2.4)),
             step(vec3(.04045), c));
}
float hash(vec3 p) {
  vec3 q = fract(p * vec3(.1031, .1030, .0973));
  q += dot(q, q.yzx + 33.33);
  return fract((q.x + q.y) * q.z);
}
float noise(vec3 p) {
  vec3 i = floor(p), f = fract(p);
  f = f * f * (3. - 2. * f);
  return mix(mix(mix(hash(i), hash(i + vec3(1, 0, 0)), f.x),
                 mix(hash(i + vec3(0, 1, 0)), hash(i + vec3(1, 1, 0)), f.x),
                 f.y),
             mix(mix(hash(i + vec3(0, 0, 1)), hash(i + vec3(1, 0, 1)), f.x),
                 mix(hash(i + vec3(0, 1, 1)), hash(i + vec3(1)), f.x), f.y),
             f.z);
}
float visibility(vec3 n) {
  if (shadows == 0)
    return 1.;
  vec3 p = lightPosition.xyz / lightPosition.w * .5 + .5;
  if (p.z > 1. || p.z < 0. || p.x < 0. || p.x > 1. || p.y < 0. || p.y > 1.)
    return 1.;
  float bias = max(.0002, .0007 * (1. - max(dot(n, sunDirection), 0.)));
  vec2 texel = 1. / vec2(textureSize(shadowMap, 0));
  float shadow = 0.;
  for (int x = -1; x <= 1; ++x)
    for (int y = -1; y <= 1; ++y)
      shadow += p.z - bias > texture(shadowMap, p.xy + vec2(x, y) * texel).r
                    ? 1.
                    : 0.;
  return 1. - shadow / 9.;
}
vec3 sky(vec3 direction) {
  return mix(vec3(.69, .76, .81), vec3(.16, .36, .63),
             pow(clamp(direction.y, 0., 1.), .35));
}
void main() {
  if (clearanceCutaway == 1 && (part == 0 || part == 3 || part == 7) &&
      worldPosition.y > cutawayY)
    discard;
  if (lit == 0) {
    outputColor = vec4(linearColor(color), 1);
    return;
  }
  vec3 n = normalize(N);
  if (!gl_FrontFacing)
    n = -n;
  vec3 v = normalize(eye - worldPosition), l = normalize(sunDirection),
       h = normalize(v + l);
  vec3 base = linearColor(clamp(color * tint, vec3(0), vec3(1)));
  float roughness = .65, metallic = 0.;
  float grain = 0.;
  if (part == 6 || part == 103 || part == 104 || part == 100 || part == 101 ||
      part == 105 || part == 106 || part == 111 || (hieu == 1 && part == 0))
    grain = noise(worldPosition * 1.5);
  if (part == 0 || part == 1 || part == 7) {
    roughness = (hieu == 1 && part != 7) ? .82 : .48;
  }
  if (part == 2 || part == 3 || part == 5 || part == 109) {
    roughness = .4;
    metallic = .45;
  }
  if (part == 4 || part == 102) {
    roughness = .96;
    base *= .92 + .16 * noise(worldPosition * 3.);
  }
  if (part == 6 || (hieu == 1 && part == 0) || part == 103) {
    roughness = .87;
    base *= .98 + .035 * grain;
  }
  if (part == 100 || part == 101) {
    float slope = 1. - clamp(n.y, 0., 1.);
    vec3 rock = linearColor(vec3(.40, .39, .35)),
         soil = linearColor(vec3(.42, .36, .25));
    base = mix(base, soil, smoothstep(.12, .5, slope) * .55);
    base = mix(base, rock, smoothstep(.3, .7, slope));
    base *= .8 + .35 * noise(worldPosition * .12) + .08 * grain;
    roughness = .95;
  }
  if (part == 104) {
    base *= .8 + .3 * noise(worldPosition * .55) +
            .12 * hash(floor(worldPosition * 3.));
    roughness = .93;
  }
  if (part == 106) {
    roughness = .85;
    base *= .985 + .025 * grain;
  }
  if (part == 105) {
    roughness = .95;
    base *= .88 + .2 * noise(worldPosition * .8);
  }
  if (part == 111) {
    roughness = .94;
    base *= .85 + .3 * grain;
  }
  if (part == 107 && presentation == 1) {
    vec2 xz = worldPosition.xz;
    float t = waterRipples == 1 ? time : 0.;
    float detail = 1. / (1. + length(fwidth(xz)) * .3);
    n = normalize(vec3(detail * (.015 * cos(xz.x * .19 + xz.y * .13 + t * .75) +
                                 .008 * cos(xz.x * .43 - t)),
                       1.,
                       detail * .013 * cos(xz.y * .25 - xz.x * .09 + t * .65)));
    if (waterRipples == 0)
      n = vec3(0, 1, 0);
    float facing = max(dot(n, v), 0.);
    float fresnel = .025 + .975 * pow(1. - facing, 5.);
    vec3 reflection = reflect(-v, n);
    base = mix(base * .75, sky(reflection), fresnel * .75);
    float sunGlint = pow(max(dot(reflect(-l, n), v), 0.), 120.) * .65;
    float sunVisibility = visibility(n);
    vec3 water = base * (.78 + .22 * sunVisibility) +
                 sunGlint * vec3(1., .9, .73) * sunVisibility;
    float fog = 1. - exp(-max(length(eye - worldPosition) - 400., 0.) / 900.);
    water = mix(water, sky(vec3(0)), fog);
    water = mix(water, sky(vec3(0)), environmentFade);
    outputColor = vec4(water, 1);
    return;
  }
  float ndl = max(dot(n, l), 0.), ndv = max(dot(n, v), .001),
        ndh = max(dot(n, h), 0.);
  float a = roughness * roughness, a2 = a * a,
        denom = ndh * ndh * (a2 - 1.) + 1.;
  float distribution = a2 / (PI * denom * denom + .0001);
  float k = (roughness + 1.) * (roughness + 1.) / 8.;
  float geometry = ndv / (ndv * (1. - k) + k) * ndl / (ndl * (1. - k) + k);
  vec3 f0 = mix(vec3(.04), base, metallic),
       fresnel = f0 + (1. - f0) * pow(1. - max(dot(h, v), 0.), 5.);
  vec3 specular =
      distribution * geometry * fresnel / (4. * ndv * max(ndl, .001) + .0001);
  vec3 diffuse = (1. - fresnel) * (1. - metallic) * base / PI;
  vec3 hemi = mix(vec3(.15, .14, .12), vec3(.39, .47, .59), n.y * .5 + .5);
  vec3 radiance = base * hemi * .65 + (diffuse + specular) *
                                          vec3(3.1, 2.9, 2.65) * ndl *
                                          visibility(n);
  if (presentation == 0)
    radiance = base * (.48 + .75 * ndl);
  float fog =
      presentation == 1
          ? 1. - exp(-max(length(eye - worldPosition) - 400., 0.) / 900.)
          : 0.;
  radiance = mix(radiance, sky(vec3(0)), fog);
  if (part >= 100)
    radiance =
        mix(radiance,
            presentation == 1 ? sky(vec3(0)) : linearColor(vec3(.12, .15, .18)),
            environmentFade);
  outputColor = vec4(radiance, 1.);
}
