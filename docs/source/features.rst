Features
========

Supported features:

- Unidirectional Path Tracing
- Stochastic Progressive Photon Mapping
- Multi-pass rendering
- Interactive rendering
- Subsurface scattering
- Depth of field
- Transformation motion blur
- Deformation motion blur
- OSL
- Physically-based sky
- Physically-based sun
- Image based lighting

Supported Maya features:

- Spot light
- Point light
- Directional light
- Area light
- Particle instancer
- True instances
- Polygon geometry including smooth preview subdivisions
- Per-face shading
- IPR
- Render region in normal and IPR mode

Supported Maya shading nodes:

- blendColors
- bulge
- bump2d
- checker
- clamp
- condition
- contrast
- file (including UDIM tiling)
- gammaCorrect
- grid
- hsvToRgb
- lambert (limited)
- rgbToHsv
- luminance
- multiplyDivide
- place2dTexture
- place3dTexture
- projection
- ramp
- reverse
- rgbToHsv
- samplerInfo
- setRange

The shading node implementation is work in progress and some nodes are only partially supported. e.g. the color space in the file node is not yet recognized.

Extra shading nodes:

- uberShader
- asLayeredShader
- asDisneyMaterial
