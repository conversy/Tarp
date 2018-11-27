/*
Tarp - v0.1.5
~~~~~~~~~~~~~~~~~~~~~~~~~~
Tarp is an almost single header C library to raster vector graphics.
It provides a lightweight and portable API purely focussed on decently
fast rendering without any gimmicks.

How to use
~~~~~~~~~~~~~~~~~~~~~~~~~~
Install Tarp or add the Tarp folder to your source directory. Include it into
your project with #include <Tarp/Tarp.h>. Specify the implementation you want to
compile in one c/c++ file to create the implementation, i.e.:

!!!ONLY DEFINE THIS ONCE, IN ONE C/C++ FILE!!!
#define TARP_IMPLEMENTATION_OPENGL
#include <Tarp/Tarp.h>

Contributors <3
~~~~~~~~~~~~~~~~~~~~~~~~~~
Tilmann Rübbelke: Radial Gradients, bug fixes, ideas
ChengCat (Github): Bug fixes, ideas

Change History
~~~~~~~~~~~~~~~~~~~~~~~~~~
v0.1.5 (11/20/2018):
- Added Meson as an alternative to CMake to build the examples.

v0.1.4 (11/06/2018):
- Added tpBeginClippingFromRenderCache.
- Made clipping stack in opengl implementation render cache based and fully immutable to guarantee
correct clipping masks in all cases (fingers crossed).

v0.1.3 (10/31/2018):
- Many Bug Fixes mainly related to clipping
- Added tpRenderCache API to allow manual control for caching. (Useful if you want to draw the same
path multiple times using different styles / transforms).
- Fixed issues with TARP_HANDLE_FUNCTIONS* macros generating multiple symbols (they were not hidden
behind TARP_IMPLEMENTATION_*)

v0.1.2 (05/30/2018): Radial Gradients, bug fixes, removed tpStyle API in favor
of a purely data based approach (see tpStyle struct). v0.1.1 (05/18/2018):
Initial release.

LICENSE
~~~~~~~~~~~~~~~~~~~~~~~~~~
See end of file.
*/

#ifndef TARP_TARP_H
#define TARP_TARP_H

/* @TODO: Double check which ones of these we actually need */
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* debug */
#if !defined(NDEBUG)
#define TARP_DEBUG
#else
#undef TARP_DEBUG
#endif

/* visibility */
/*
You can overwrite these (i.e. if your comiler does not support them or you want
different visibility), by simply defining them before including tarp.
*/
#ifndef TARP_API
#define TARP_API __attribute__((visibility("default")))
#define TARP_LOCAL __attribute__((visibility("hidden")))
#endif

/* thread local storage. Only used for error variable */
#if defined(__GNUC__)
#define TARP_TLS __thread
#elif defined(_MSC_VER)
#define TARP_TLS __declspec(thread)
#endif

/*
memory allocation, you can define you own before including tarp for custom
memory allocation!
*/
#ifndef TARP_MALLOC
#define TARP_MALLOC(_bc) malloc(_bc)
#define TARP_REALLOC(_ptr, _bc) realloc(_ptr, _bc)
#define TARP_FREE(_ptr) free(_ptr)
#endif

#ifdef TARP_IMPLEMENTATION_OPENGL
#ifdef TARP_DEBUG
#define _TARP_ASSERT_NO_GL_ERROR(_func)                                                            \
    do                                                                                             \
    {                                                                                              \
        GLenum glerr;                                                                              \
        _func;                                                                                     \
        glerr = glGetError();                                                                      \
        if (glerr != GL_NO_ERROR)                                                                  \
        {                                                                                          \
            switch (glerr)                                                                         \
            {                                                                                      \
            case GL_NO_ERROR:                                                                      \
                fprintf(stderr,                                                                    \
                        "%s line %i GL_NO_ERROR: No error has been recorded.\n",                   \
                        __FILE__,                                                                  \
                        __LINE__);                                                                 \
                break;                                                                             \
            case GL_INVALID_ENUM:                                                                  \
                fprintf(stderr,                                                                    \
                        "%s line %i GL_INVALID_ENUM: An unacceptable value "                       \
                        "is specified for an enumerated argument. The "                            \
                        "offending command is ignored and has no other side "                      \
                        "effect than to set the error flag.\n",                                    \
                        __FILE__,                                                                  \
                        __LINE__);                                                                 \
                break;                                                                             \
            case GL_INVALID_VALUE:                                                                 \
                fprintf(stderr,                                                                    \
                        "%s line %i GL_INVALID_VALUE: A numeric argument is out "                  \
                        "of range. The offending command is ignored and has no "                   \
                        "other side effect than to set the error flag.\n",                         \
                        __FILE__,                                                                  \
                        __LINE__);                                                                 \
                break;                                                                             \
            case GL_INVALID_OPERATION:                                                             \
                fprintf(stderr,                                                                    \
                        "%s line %i GL_INVALID_OPERATION: The specified "                          \
                        "operation is not allowed in the current state. The "                      \
                        "offending command is ignored and has no other side "                      \
                        "effect than to set the error flag.\n",                                    \
                        __FILE__,                                                                  \
                        __LINE__);                                                                 \
                break;                                                                             \
            case GL_INVALID_FRAMEBUFFER_OPERATION:                                                 \
                fprintf(stderr,                                                                    \
                        "%s line %i GL_INVALID_FRAMEBUFFER_OPERATION: The "                        \
                        "framebuffer object is not complete. The offending "                       \
                        "command is ignored and has no other side effect "                         \
                        "than to set the error flag.\n",                                           \
                        __FILE__,                                                                  \
                        __LINE__);                                                                 \
                break;                                                                             \
            case GL_OUT_OF_MEMORY:                                                                 \
                fprintf(stderr,                                                                    \
                        "%s line %i GL_OUT_OF_MEMORY: There is not enough "                        \
                        "memory left to executeLua the command. The state of "                     \
                        "the GL is undefined, except for the state of the "                        \
                        "error flags, after this error is recorded.\n",                            \
                        __FILE__,                                                                  \
                        __LINE__);                                                                 \
                break;                                                                             \
            }                                                                                      \
            exit(EXIT_FAILURE);                                                                    \
        }                                                                                          \
    } while (0)
#else
#define _TARP_ASSERT_NO_GL_ERROR(_func) _func
#endif /* TARP_DEBUG */

/* some tarp related opengl related settings */
#define TARP_GL_RAMP_TEXTURE_SIZE 1024
#define TARP_GL_MAX_CLIPPING_STACK_DEPTH 64
#define TARP_GL_ERROR_MESSAGE_SIZE 512

#endif /* TARP_IMPLEMENTATION_OPENGL */

/* some settings that you most likely won't have to touch*/
#define TARP_MAX_COLOR_STOPS 128
#define TARP_MAX_DASH_ARRAY_SIZE 64
#define TARP_MAX_ERROR_MESSAGE 512
#define TARP_MAX_CURVE_SUBDIVISIONS 16
#define TARP_RADIAL_GRADIENT_SLICES 64

/* some helper macros */
#define TARP_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define TARP_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define TARP_CLAMP(a, l, h) TARP_MAX(TARP_MIN(a, h), l)

/* some constants */
#define TARP_KAPPA 0.55228474983f
#define TARP_PI 3.14159265358979323846f
#define TARP_HALF_PI (TARP_PI * 0.5f)

/* define TARP_IMPLEMENTATION if any implementation is defined */
#ifdef TARP_IMPLEMENTATION_OPENGL
#define TARP_IMPLEMENTATION
#endif /* TARP_IMPLEMENTATION_OPENGL */

/*
helper to generate a typesafe handle class.
*/
#define TARP_HANDLE(_t)                                                                            \
    typedef struct TARP_API                                                                        \
    {                                                                                              \
        void * pointer;                                                                            \
    } _t

#define TARP_HANDLE_FUNCTIONS_DEF(_t)                                                              \
    TARP_API tpBool _t##IsValidHandle(_t _val);                                                    \
    TARP_API _t _t##InvalidHandle();

#define TARP_HANDLE_FUNCTIONS_DECL(_t)                                                             \
    TARP_API tpBool _t##IsValidHandle(_t _val)                                                     \
    {                                                                                              \
        return (tpBool)(_val.pointer != NULL);                                                     \
    }                                                                                              \
    TARP_API _t _t##InvalidHandle()                                                                \
    {                                                                                              \
        _t ret = { NULL };                                                                         \
        return ret;                                                                                \
    }

#ifdef __cplusplus
extern "C" {
#endif

/*
Constants
~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
typedef enum TARP_API
{
    kTpGradientTypeLinear,
    kTpGradientTypeRadial
} tpGradientType;

typedef enum TARP_API
{
    kTpPaintTypeNone,
    kTpPaintTypeColor,
    kTpPaintTypeGradient
} tpPaintType;

typedef enum TARP_API
{
    kTpFillRuleEvenOdd,
    kTpFillRuleNonZero
} tpFillRule;

typedef enum TARP_API
{
    kTpStrokeCapRound,
    kTpStrokeCapSquare,
    kTpStrokeCapButt
} tpStrokeCap;

typedef enum
{
    kTpStrokeJoinMiter,
    kTpStrokeJoinRound,
    kTpStrokeJoinBevel
} tpStrokeJoin;

/*
Basic Types
~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
typedef float tpFloat;
typedef enum TARP_API
{
    tpFalse = 0,
    tpTrue = 1
} tpBool;

TARP_HANDLE(tpPath);
TARP_HANDLE(tpRenderCache);
TARP_HANDLE(tpGradient);

/*
Structures
~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
typedef struct TARP_API
{
    tpFloat r, g, b, a;
} tpColor;

typedef struct TARP_API
{
    tpFloat x;
    tpFloat y;
} tpVec2;

typedef struct TARP_API
{
    tpFloat v[4];
} tpMat2;

typedef struct TARP_API
{
    tpMat2 m;
    tpVec2 t;
} tpTransform;

typedef struct TARP_API
{
    tpFloat v[16];
} tpMat4;

typedef struct TARP_API
{
    tpColor color;
    tpFloat offset;
} tpColorStop;

typedef struct TARP_API
{
    tpVec2 handleIn;
    tpVec2 position;
    tpVec2 handleOut;
} tpSegment;

typedef union TARP_API
{
    tpGradient gradient;
    tpColor color;
} _tpPaintUnion;

typedef struct TARP_API
{
    _tpPaintUnion data;
    tpPaintType type;
} tpPaint;

typedef struct TARP_API
{
    tpPaint fill;
    tpPaint stroke;
    tpFloat strokeWidth;
    tpStrokeCap strokeCap;
    tpStrokeJoin strokeJoin;
    tpFillRule fillRule;
    const tpFloat * dashArray;
    int dashCount;
    tpFloat dashOffset;
    tpFloat miterLimit;
    tpBool scaleStroke;
} tpStyle;

TARP_HANDLE(tpContext);

/*
NOTE: All the color, matrix and vector functions are mainly for internal use
The rotation/scale/projection functionality are mainly provided to be used
in the examples.
*/

/*
Color Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
TARP_API tpColor tpColorMake(tpFloat _r, tpFloat _g, tpFloat _b, tpFloat _a);

/*
Vector Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
TARP_API tpVec2 tpVec2Make(tpFloat _x, tpFloat _y);

TARP_API tpVec2 tpVec2Add(tpVec2 _a, tpVec2 _b);

TARP_API tpVec2 tpVec2Sub(tpVec2 _a, tpVec2 _b);

TARP_API tpVec2 tpVec2Mult(tpVec2 _a, tpVec2 _b);

TARP_API tpVec2 tpVec2MultScalar(tpVec2 _a, tpFloat _b);

TARP_API tpVec2 tpVec2Div(tpVec2 _a, tpVec2 _b);

TARP_API tpBool tpVec2Equals(tpVec2 _a, tpVec2 _b);

TARP_API tpFloat tpVec2Length(tpVec2 _vec);

TARP_API tpFloat tpVec2LengthSquared(tpVec2 _vec);

TARP_API tpFloat tpVec2Dot(tpVec2 _a, tpVec2 _b);

TARP_API tpFloat tpVec2Cross(tpVec2 _a, tpVec2 _b);

TARP_API void tpVec2NormalizeSelf(tpVec2 * _vec);

TARP_API tpVec2 tpVec2Normalize(tpVec2 _vec);

TARP_API tpVec2 tpVec2Perp(tpVec2 _a);

TARP_API tpFloat tpVec2Distance(tpVec2 _a, tpVec2 _b);

TARP_API tpFloat tpVec2DistanceSquared(tpVec2 _a, tpVec2 _b);

TARP_API tpVec2 tpVec2Lerp(tpVec2 _a, tpVec2 _b, tpFloat _t);

/*
Matrix Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
TARP_API tpMat2 tpMat2Make(tpFloat _a,
                           tpFloat _b, /* row1 */
                           tpFloat _c,
                           tpFloat _d); /* row2 */

TARP_API tpMat2 tpMat2MakeIdentity();

TARP_API tpMat2 tpMat2MakeScale(tpFloat _x, tpFloat _y);

TARP_API tpMat2 tpMat2MakeSkew(tpFloat _x, tpFloat _y);

TARP_API tpMat2 tpMat2MakeRotation(tpFloat _angle);

TARP_API int tpMat2Decompose(const tpMat2 * _mat,
                             tpVec2 * _outScale,
                             tpVec2 * _outSkew,
                             tpFloat * _outRotation);

TARP_API tpBool tpMat2Equals(const tpMat2 * _a, const tpMat2 * _b);

TARP_API tpVec2 tpMat2MultVec2(const tpMat2 * _mat, tpVec2 _vec);

TARP_API tpMat2 tpMat2Mult(const tpMat2 * _a, const tpMat2 * _b);

TARP_API tpMat2 tpMat2Invert(const tpMat2 * _mat);

TARP_API tpTransform
tpTransformMake(tpFloat _a, tpFloat _b, tpFloat _x, tpFloat _c, tpFloat _d, tpFloat _y);

TARP_API tpTransform tpTransformMakeIdentity();

TARP_API tpTransform tpTransformMakeTranslation(tpFloat _x, tpFloat _y);
    
TARP_API tpTransform tpTransformMakeScale(tpFloat _x, tpFloat _y);

TARP_API tpTransform tpTransformMakeSkew(tpFloat _x, tpFloat _y);

TARP_API tpTransform tpTransformMakeRotation(tpFloat _angle);

TARP_API int tpTransformDecompose(const tpTransform * _trafo,
                                  tpVec2 * _outTranslation,
                                  tpVec2 * _outScale,
                                  tpVec2 * _outSkew,
                                  tpFloat * _outRotation);

TARP_API tpBool tpTransformEquals(const tpTransform * _a, const tpTransform * _b);

TARP_API tpVec2 tpTransformApply(const tpTransform * _trafo, tpVec2 _vec);

TARP_API tpTransform tpTransformCombine(const tpTransform * _a, const tpTransform * _b);

TARP_API tpMat4 tpMat4Make(tpFloat _v0,
                           tpFloat _v1,
                           tpFloat _v2,
                           tpFloat _v3, /* row1 */
                           tpFloat _v4,
                           tpFloat _v5,
                           tpFloat _v6,
                           tpFloat _v7, /* row2 */
                           tpFloat _v8,
                           tpFloat _v9,
                           tpFloat _v10,
                           tpFloat _v11, /* row3 */
                           tpFloat _v12,
                           tpFloat _v13,
                           tpFloat _v14,
                           tpFloat _v15); /* row4 */

TARP_API tpMat4 tpMat4MakeIdentity();

TARP_API tpMat4 tpMat4MakeOrtho(
    tpFloat _left, tpFloat _right, tpFloat _bottom, tpFloat _top, tpFloat _near, tpFloat _far);

TARP_API tpMat4 tpMat4MakeFrom2DTransform(const tpTransform * _transform);

TARP_API tpMat4 tpMat4Mult(const tpMat4 * _a, const tpMat4 * _b);

/*
Segment Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
/* creates a segment, you rarely need to call this directly */
TARP_API tpSegment
tpSegmentMake(tpFloat _h0x, tpFloat _h0y, tpFloat _px, tpFloat _py, tpFloat _h1x, tpFloat _h1y);

/*
Path Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
TARP_API tpPath tpPathCreate();

TARP_API tpPath tpPathClone(tpPath _path);

TARP_API void tpPathDestroy(tpPath _path);

/* Set the fill transformation */
TARP_API tpBool tpPathSetFillPaintTransform(tpPath, const tpTransform * _transform);

/* Set the stroke transformation */
TARP_API tpBool tpPathSetStrokePaintTransform(tpPath, const tpTransform * _transform);

/* Adds a circle contour to the provided path */
TARP_API tpBool tpPathAddCircle(tpPath _path, tpFloat _x, tpFloat _y, tpFloat _r);

/* Adds an ellipse contour to the provided path */
TARP_API tpBool
tpPathAddEllipse(tpPath _path, tpFloat _x, tpFloat _y, tpFloat _width, tpFloat _height);

/* Adds a rectangle contour to the provided path */
TARP_API tpBool
tpPathAddRect(tpPath _path, tpFloat _x, tpFloat _y, tpFloat _width, tpFloat _height);

/* Adds a segment to the current path contour */
TARP_API tpBool tpPathAddSegment(
    tpPath _path, tpFloat _h0x, tpFloat _h0y, tpFloat _px, tpFloat _py, tpFloat _h1x, tpFloat _h1y);

/* Starts a new contour at x, y */
TARP_API tpBool tpPathMoveTo(tpPath _path, tpFloat _x, tpFloat _y);

/* Connects the last segment of the current contour with x, y in a straight
 * line */
TARP_API tpBool tpPathLineTo(tpPath _path, tpFloat _x, tpFloat _y);

/* Connects the last segment of the current contour with a cubic bezier
 * curve to x, y */
TARP_API tpBool tpPathCubicCurveTo(
    tpPath _path, tpFloat _h0x, tpFloat _h0y, tpFloat _h1x, tpFloat _h1y, tpFloat _px, tpFloat _py);

/* Connects the last segment of the current contour with a quadratic bezier
 * curve to x, y */
TARP_API tpBool
tpPathQuadraticCurveTo(tpPath _path, tpFloat _hx, tpFloat _hy, tpFloat _px, tpFloat _py);

/* Connects the last segment of the current contour with an arc
 * to x, y */
TARP_API tpBool
tpPathArcTo(tpPath _path, tpFloat _rx, tpFloat _ry, tpFloat _rotx, tpFloat _fl, tpFloat _swfl, tpFloat _x, tpFloat _y);
    
/* Closes the current contour */
TARP_API tpBool tpPathClose(tpPath _path);

/* Removes all contours from the path */
TARP_API tpBool tpPathClear(tpPath _path);

/* Removes a contour by index */
TARP_API tpBool tpPathRemoveContour(tpPath _path, int _contourIndex);

/* Returns the number of contours */
TARP_API int tpPathContourCount(tpPath _path);

/* Removes one segment from a contour */
TARP_API tpBool tpPathRemoveSegment(tpPath _path, int _contourIndex, int _segmentIndex);

/* Removes a range of segments from a contour */
TARP_API tpBool tpPathRemoveSegments(tpPath _path, int _contourIndex, int _from, int _to);

/* Adds segments to the current contour */
TARP_API tpBool tpPathAddSegments(tpPath _path, const tpSegment * _segments, int _count);

/* Adds a new contour to the path from the provided segments */
TARP_API tpBool tpPathAddContour(tpPath _path,
                                 const tpSegment * _segments,
                                 int _count,
                                 tpBool _bClosed);

/*
Replaces a contour in the path with the provided segments.
If contourIndex is > than the number of existing number of contours,
a new contour will be added to the path.
 */
TARP_API tpBool tpPathSetContour(
    tpPath _path, int _contourIndex, const tpSegment * _segments, int _count, tpBool _bClosed);

/* generates tpPathInvalidHandle() and tpPathIsValidHandle(tpPath) functions
to generate an invalid handle and check a handle for validity. */
TARP_HANDLE_FUNCTIONS_DEF(tpPath)

/*
Paint Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

/* creates a color paint */
TARP_API tpPaint tpPaintMakeColor(tpFloat _r, tpFloat _g, tpFloat _b, tpFloat _a);

/* creates a gradient paint */
TARP_API tpPaint tpPaintMakeGradient(tpGradient _gradient);

/*
Style Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

/* default initializes and returns a tpStyle struct. */
TARP_API tpStyle tpStyleMake();

/*
Gradient Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
/* Creates a linear gradient with the origin at x0, y0 and the destination
 * at x1, y1 */
TARP_API tpGradient tpGradientCreateLinear(tpFloat _x0, tpFloat _y0, tpFloat _x1, tpFloat _y1);

/* Creates a radial gradient starting at fx, fy and running towards an
ellipse with one semi-axis going from ox, oy, to x1, y1 and the other axis
being scaled by the ratio */
TARP_API tpGradient tpGradientCreateRadial(
    tpFloat _fx, tpFloat _fy, tpFloat _ox, tpFloat _oy, tpFloat _dx, tpFloat _dy, tpFloat _ratio);

/* Creates a symmetric radial gradient with radius r at x, y */
TARP_API tpGradient tpGradientCreateRadialSymmetric(tpFloat _x, tpFloat _y, tpFloat _r);

/* Creates a copy of a gradient */
TARP_API tpGradient tpGradientClone(tpGradient _gradient);

/* Sets the origin to x0, y0 and the destination at x1, y1 */
TARP_API void tpGradientSetPositions(
    tpGradient _gradient, tpFloat _x0, tpFloat _y0, tpFloat _x1, tpFloat _y1);

/* Sets the focal point of a radial gradient to a position relative to the
 * gradient's origin */
TARP_API void tpGradientSetFocalPointOffset(tpGradient _gradient, tpFloat _x, tpFloat _y);

/* Sets the ratio between the minor and major axis of an elliptical radial
 * gradient */
TARP_API void tpGradientSetRatio(tpGradient _gradient, tpFloat _ratio);

/*
Adds a color stop to the gradient. Offset is in the range 0-1 where 0
positions the color stop at the origin of the gradient and 1.0 at the
destination.
*/
TARP_API void tpGradientAddColorStop(
    tpGradient _gradient, tpFloat _r, tpFloat _g, tpFloat _b, tpFloat _a, tpFloat _offset);

/* remove all color stops */
TARP_API void tpGradientClearColorStops(tpGradient _gradient);

TARP_API void tpGradientDestroy(tpGradient _gradient);

/* generates tpGradientInvalidHandle() and
tpGradientIsValidHandle(tpGradient) functions to generate
an invalid handle and check a handle for validity. */
TARP_HANDLE_FUNCTIONS_DEF(tpGradient)

/*
Context Related Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

/* Call this to initialize a tarp context. */
TARP_API tpContext tpContextCreate();

/* shut down a context */
TARP_API void tpContextDestroy(tpContext _ctx);

/* Call this at the beginning of a "frame" before you call any draw
 * functions */
TARP_API tpBool tpPrepareDrawing(tpContext _ctx);

/* Call this at the end of a "frame" after you are done drawing */
TARP_API tpBool tpFinishDrawing(tpContext _ctx);

/* Set the projection */
TARP_API tpBool tpSetProjection(tpContext _ctx, const tpMat4 * _projection);

/* Set the path transformation. All following draw calls will be affected by
 * it. */
TARP_API tpBool tpSetTransform(tpContext _ctx, const tpTransform * _transform);

/* Reset the transformation to the identity matrix */
TARP_API tpBool tpResetTransform(tpContext _ctx);

/* Draw a path with the provided style */
TARP_API tpBool tpDrawPath(tpContext _ctx, tpPath _path, const tpStyle * _style);

TARP_API tpRenderCache tpRenderCacheCreate();

TARP_API tpBool tpCachePath(tpContext _ctx,
                            tpPath _path,
                            const tpStyle * _style,
                            tpRenderCache _cache);

TARP_API void tpRenderCacheDestroy(tpRenderCache _cache);

TARP_API tpBool tpDrawRenderCache(tpContext _ctx, tpRenderCache _cache);

TARP_HANDLE_FUNCTIONS_DEF(tpRenderCache)

/*
Define a clipping path. You can nest these calls. All following draw
calls will be clippied by the provided path.
*/
TARP_API tpBool tpBeginClipping(tpContext _ctx, tpPath _path);

TARP_API tpBool tpBeginClippingWithFillRule(tpContext _ctx, tpPath _path, tpFillRule _rule);

TARP_API tpBool tpBeginClippingFromRenderCache(tpContext _ctx, tpRenderCache _cache);
/* NOTE: There is no tpBeginClippingFromRenderCache that allows you to set the fillRule as a render
 * cache is immutable and contains its style/fillRule */

/* End the most recent clipping path */
TARP_API tpBool tpEndClipping(tpContext _ctx);

/* End all clipping paths. This will remove all clipping. */
TARP_API tpBool tpResetClipping(tpContext _ctx);

/* Returns a string identifier of the current implementation */
TARP_API const char * tpImplementationName();

/* generates tpContextInvalidHandle() and tpContextIsValidHandle(tpContext)
functions to generate an invalid handle and check a handle for validity. */
TARP_HANDLE_FUNCTIONS_DEF(tpContext)

/* retrieves a context error message  */
TARP_API const char * tpErrorMessage();

#ifdef TARP_IMPLEMENTATION

/*
Implementations
~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

TARP_HANDLE_FUNCTIONS_DECL(tpPath)
TARP_HANDLE_FUNCTIONS_DECL(tpGradient)
TARP_HANDLE_FUNCTIONS_DECL(tpRenderCache)
TARP_HANDLE_FUNCTIONS_DECL(tpContext)

TARP_API tpColor tpColorMake(tpFloat _r, tpFloat _g, tpFloat _b, tpFloat _a)
{
    tpColor ret;
    ret.r = _r;
    ret.g = _g;
    ret.b = _b;
    ret.a = _a;
    return ret;
}

TARP_API tpVec2 tpVec2Make(tpFloat _x, tpFloat _y)
{
    tpVec2 ret;
    ret.x = _x;
    ret.y = _y;
    return ret;
}

TARP_API tpVec2 tpVec2Add(tpVec2 _a, tpVec2 _b)
{
    tpVec2 ret;
    ret.x = _a.x + _b.x;
    ret.y = _a.y + _b.y;
    return ret;
}

TARP_API tpVec2 tpVec2Sub(tpVec2 _a, tpVec2 _b)
{
    tpVec2 ret;
    ret.x = _a.x - _b.x;
    ret.y = _a.y - _b.y;
    return ret;
}

TARP_API tpVec2 tpVec2Mult(tpVec2 _a, tpVec2 _b)
{
    tpVec2 ret;
    ret.x = _a.x * _b.x;
    ret.y = _a.y * _b.y;
    return ret;
}

TARP_API tpVec2 tpVec2MultScalar(tpVec2 _a, tpFloat _b)
{
    tpVec2 ret;
    ret.x = _a.x * _b;
    ret.y = _a.y * _b;
    return ret;
}

TARP_API tpVec2 tpVec2Div(tpVec2 _a, tpVec2 _b)
{
    tpVec2 ret;
    ret.x = _a.x / _b.x;
    ret.y = _a.y / _b.y;
    return ret;
}

TARP_API tpBool tpVec2Equals(tpVec2 _a, tpVec2 _b)
{
    return (tpBool)(_a.x == _b.x && _a.y == _b.y);
}

TARP_API tpFloat tpVec2Length(tpVec2 _vec)
{
    return sqrt(_vec.x * _vec.x + _vec.y * _vec.y);
}

TARP_API tpFloat tpVec2LengthSquared(tpVec2 _vec)
{
    return _vec.x * _vec.x + _vec.y * _vec.y;
}

TARP_API void tpVec2NormalizeSelf(tpVec2 * _vec)
{
    tpFloat s = 1 / tpVec2Length(*_vec);
    _vec->x *= s;
    _vec->y *= s;
}

TARP_API tpVec2 tpVec2Normalize(tpVec2 _vec)
{
    tpVec2 ret = _vec;
    tpFloat s = 1 / tpVec2Length(_vec);
    ret.x *= s;
    ret.y *= s;
    return ret;
}

TARP_API tpVec2 tpVec2Perp(tpVec2 _a)
{
    tpVec2 ret;
    ret.x = _a.y;
    ret.y = -_a.x;
    return ret;
}

TARP_API tpFloat tpVec2Dot(tpVec2 _a, tpVec2 _b)
{
    return _a.x * _b.x + _a.y * _b.y;
}

TARP_API tpFloat tpVec2Cross(tpVec2 _a, tpVec2 _b)
{
    return _a.x * _b.y - _a.y * _b.x;
}

TARP_API tpFloat tpVec2Distance(tpVec2 _a, tpVec2 _b)
{
    return tpVec2Length(tpVec2Sub(_a, _b));
}

TARP_API tpFloat tpVec2DistanceSquared(tpVec2 _a, tpVec2 _b)
{
    return tpVec2LengthSquared(tpVec2Sub(_a, _b));
}

TARP_API tpVec2 tpVec2Lerp(tpVec2 _a, tpVec2 _b, tpFloat _t)
{
    return tpVec2Add(tpVec2MultScalar(_a, 1.0f - _t), tpVec2MultScalar(_b, _t));
}

TARP_API tpMat2 tpMat2Make(tpFloat _a, tpFloat _b, tpFloat _c, tpFloat _d)
{
    tpMat2 ret;
    ret.v[0] = _a;
    ret.v[2] = _b;
    ret.v[1] = _c;
    ret.v[3] = _d;
    return ret;
}

TARP_API tpMat2 tpMat2MakeIdentity()
{
    return tpMat2Make(1, 0, 0, 1);
}

TARP_API tpMat2 tpMat2MakeScale(tpFloat _x, tpFloat _y)
{
    return tpMat2Make(_x, 0, 0, _y);
}

TARP_API tpMat2 tpMat2MakeSkew(tpFloat _x, tpFloat _y)
{
    return tpMat2Make(1, tan(_x), tan(_y), 1);
}

TARP_API tpMat2 tpMat2MakeRotation(tpFloat _angle)
{
    tpFloat c = cos(_angle);
    tpFloat s = sin(_angle);
    return tpMat2Make(c, -s, s, c);
}

TARP_API int tpMat2Decompose(const tpMat2 * _mat,
                             tpVec2 * _outScale,
                             tpVec2 * _outSkew,
                             tpFloat * _outRotation)
{
    tpFloat a, b, c, d, det;

    a = _mat->v[0];
    b = _mat->v[2];
    c = _mat->v[1];
    d = _mat->v[3];
    det = a * d - b * c;

    if (a != 0 || b != 0)
    {
        tpFloat r = sqrt(a * a + b * b);
        *_outRotation = acos(a / r) * (b > 0 ? 1 : -1);
        _outScale->x = r;
        _outScale->y = det / r;
        _outSkew->x = atan2(a * c + b * d, r * r);
        _outSkew->y = 0;
    }
    else if (c != 0 || d != 0)
    {
        tpFloat s = sqrt(c * c + d * d);
        *_outRotation = asin(c / s) * (d > 0 ? 1 : -1);
        _outScale->x = det / s;
        _outScale->y = s;
        _outSkew->x = 0;
        _outSkew->y = atan2(a * c + b * d, s * s);
    }
    else /* a = b = c = d = 0 */
    {
        *_outRotation = 0;
        _outScale->x = 0;
        _outScale->y = 0;
        _outSkew->x = 0;
        _outSkew->y = 0;
    }

    return 0;
}

TARP_API tpBool tpMat2Equals(const tpMat2 * _a, const tpMat2 * _b)
{
    return (tpBool)(_a->v[0] == _b->v[0] && _a->v[1] == _b->v[1] && _a->v[2] == _b->v[2] &&
                    _a->v[3] == _b->v[3]);
}

TARP_API tpVec2 tpMat2MultVec2(const tpMat2 * _mat, tpVec2 _vec)
{
    tpVec2 ret;
    ret.x = _vec.x * _mat->v[0] + _vec.y * _mat->v[2];
    ret.y = _vec.x * _mat->v[1] + _vec.y * _mat->v[3];
    return ret;
}

TARP_API tpMat2 tpMat2Mult(const tpMat2 * _a, const tpMat2 * _b)
{
    tpMat2 ret;

    ret.v[0] = _b->v[0] * _a->v[0] + _b->v[1] * _a->v[2];
    ret.v[1] = _b->v[0] * _a->v[1] + _b->v[1] * _a->v[3];
    ret.v[2] = _b->v[2] * _a->v[0] + _b->v[3] * _a->v[2];
    ret.v[3] = _b->v[2] * _a->v[1] + _b->v[3] * _a->v[3];

    return ret;
}

TARP_API tpMat2 tpMat2Invert(const tpMat2 * _mat)
{
    tpMat2 ret;
    tpFloat inv_det = 1 / (_mat->v[0] * _mat->v[3] - _mat->v[1] * _mat->v[2]);
    ret.v[0] = _mat->v[3] * inv_det;
    ret.v[1] = -_mat->v[1] * inv_det;
    ret.v[2] = -_mat->v[2] * inv_det;
    ret.v[3] = _mat->v[0] * inv_det;
    return ret;
}

TARP_API tpTransform tpTransformInvert(const tpTransform * _trafo)
{
    tpTransform ret;
    ret.m = tpMat2Invert(&_trafo->m);
    ret.t = tpMat2MultVec2(&ret.m, tpVec2Make(-_trafo->t.x, -_trafo->t.y));
    return ret;
}

TARP_API tpTransform
tpTransformMake(tpFloat _a, tpFloat _b, tpFloat _x, tpFloat _c, tpFloat _d, tpFloat _y)
{
    tpTransform ret;
    ret.m = tpMat2Make(_a, _b, _c, _d);
    ret.t = tpVec2Make(_x, _y);
    return ret;
}

TARP_API tpTransform tpTransformMakeIdentity()
{
    tpTransform ret;
    ret.m = tpMat2MakeIdentity();
    ret.t = tpVec2Make(0, 0);
    return ret;
}

TARP_API tpTransform tpTransformMakeTranslation(tpFloat _x, tpFloat _y)
{
    tpTransform ret;
    ret.m = tpMat2MakeIdentity();
    ret.t = tpVec2Make(_x, _y);
    return ret;
}

TARP_API tpTransform tpTransformMakeScale(tpFloat _x, tpFloat _y)
{
    tpTransform ret;
    ret.m = tpMat2MakeScale(_x, _y);
    ret.t = tpVec2Make(0, 0);
    return ret;
}

TARP_API tpTransform tpTransformMakeSkew(tpFloat _x, tpFloat _y)
{
    tpTransform ret;
    ret.m = tpMat2MakeSkew(_x, _y);
    ret.t = tpVec2Make(0, 0);
    return ret;
}

TARP_API tpTransform tpTransformMakeRotation(tpFloat _angle)
{
    tpTransform ret;
    ret.m = tpMat2MakeRotation(_angle);
    ret.t = tpVec2Make(0, 0);
    return ret;
}

TARP_API int tpTransformDecompose(const tpTransform * _trafo,
                                  tpVec2 * _outTranslation,
                                  tpVec2 * _outScale,
                                  tpVec2 * _outSkew,
                                  tpFloat * _outRotation)
{
    *_outTranslation = _trafo->t;
    return tpMat2Decompose(&_trafo->m, _outScale, _outSkew, _outRotation);
}

TARP_API tpBool tpTransformEquals(const tpTransform * _a, const tpTransform * _b)
{
    return (tpBool)(tpMat2Equals(&_a->m, &_b->m) && tpVec2Equals(_a->t, _b->t));
}

TARP_API tpVec2 tpTransformApply(const tpTransform * _trafo, tpVec2 _vec)
{
    return tpVec2Add(tpMat2MultVec2(&_trafo->m, _vec), _trafo->t);
}

TARP_API tpTransform tpTransformCombine(const tpTransform * _a, const tpTransform * _b)
{
    tpTransform ret;
    ret.t = tpTransformApply(_a, _b->t);
    ret.m = tpMat2Mult(&_a->m, &_b->m);
    return ret;
}

TARP_API tpMat4 tpMat4Make(tpFloat _v0,
                           tpFloat _v1,
                           tpFloat _v2,
                           tpFloat _v3,
                           tpFloat _v4,
                           tpFloat _v5,
                           tpFloat _v6,
                           tpFloat _v7,
                           tpFloat _v8,
                           tpFloat _v9,
                           tpFloat _v10,
                           tpFloat _v11,
                           tpFloat _v12,
                           tpFloat _v13,
                           tpFloat _v14,
                           tpFloat _v15)
{
    tpMat4 ret;
    ret.v[0] = _v0;
    ret.v[1] = _v4;
    ret.v[2] = _v8;
    ret.v[3] = _v12;
    ret.v[4] = _v1;
    ret.v[5] = _v5;
    ret.v[6] = _v9;
    ret.v[7] = _v13;
    ret.v[8] = _v2;
    ret.v[9] = _v6;
    ret.v[10] = _v10;
    ret.v[11] = _v14;
    ret.v[12] = _v3;
    ret.v[13] = _v7;
    ret.v[14] = _v11;
    ret.v[15] = _v15;
    return ret;
}

TARP_API tpMat4 tpMat4MakeIdentity()
{
    return tpMat4Make(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
}

TARP_API tpMat4 tpMat4MakeOrtho(
    tpFloat _left, tpFloat _right, tpFloat _bottom, tpFloat _top, tpFloat _near, tpFloat _far)
{
    tpFloat a = 2.0 / (_right - _left);
    tpFloat b = 2.0 / (_top - _bottom);
    tpFloat c = -2.0 / (_far - _near);
    tpFloat tx = -(_right + _left) / (_right - _left);
    tpFloat ty = -(_top + _bottom) / (_top - _bottom);
    tpFloat tz = -(_far + _near) / (_far - _near);

    return tpMat4Make(a, 0.0, 0.0, tx, 0.0, b, 0.0, ty, 0.0, 0.0, c, tz, 0.0, 0.0, 0, 1.0);
}

TARP_API tpMat4 tpMat4MakeFrom2DTransform(const tpTransform * _transform)
{
    return tpMat4Make(_transform->m.v[0],
                      _transform->m.v[2],
                      0,
                      _transform->t.x,
                      _transform->m.v[1],
                      _transform->m.v[3],
                      0,
                      _transform->t.y,
                      0,
                      0,
                      1,
                      0,
                      0,
                      0,
                      0,
                      1.0);
}

TARP_API tpMat4 tpMat4Mult(const tpMat4 * _a, const tpMat4 * _b)
{
    tpMat4 ret;

    ret.v[0] =
        _b->v[0] * _a->v[0] + _b->v[1] * _a->v[4] + _b->v[2] * _a->v[8] + _b->v[3] * _a->v[12];
    ret.v[1] =
        _b->v[0] * _a->v[1] + _b->v[1] * _a->v[5] + _b->v[2] * _a->v[9] + _b->v[3] * _a->v[13];
    ret.v[2] =
        _b->v[0] * _a->v[2] + _b->v[1] * _a->v[6] + _b->v[2] * _a->v[10] + _b->v[3] * _a->v[14];
    ret.v[3] =
        _b->v[0] * _a->v[3] + _b->v[1] * _a->v[7] + _b->v[2] * _a->v[11] + _b->v[3] * _a->v[15];
    ret.v[4] =
        _b->v[4] * _a->v[0] + _b->v[5] * _a->v[4] + _b->v[6] * _a->v[8] + _b->v[7] * _a->v[12];
    ret.v[5] =
        _b->v[4] * _a->v[1] + _b->v[5] * _a->v[5] + _b->v[6] * _a->v[9] + _b->v[7] * _a->v[13];
    ret.v[6] =
        _b->v[4] * _a->v[2] + _b->v[5] * _a->v[6] + _b->v[6] * _a->v[10] + _b->v[7] * _a->v[14];
    ret.v[7] =
        _b->v[4] * _a->v[3] + _b->v[5] * _a->v[7] + _b->v[6] * _a->v[11] + _b->v[7] * _a->v[15];
    ret.v[8] =
        _b->v[8] * _a->v[0] + _b->v[9] * _a->v[4] + _b->v[10] * _a->v[8] + _b->v[11] * _a->v[12];
    ret.v[9] =
        _b->v[8] * _a->v[1] + _b->v[9] * _a->v[5] + _b->v[10] * _a->v[9] + _b->v[11] * _a->v[13];
    ret.v[10] =
        _b->v[8] * _a->v[2] + _b->v[9] * _a->v[6] + _b->v[10] * _a->v[10] + _b->v[11] * _a->v[14];
    ret.v[11] =
        _b->v[8] * _a->v[3] + _b->v[9] * _a->v[7] + _b->v[10] * _a->v[11] + _b->v[11] * _a->v[15];
    ret.v[12] =
        _b->v[12] * _a->v[0] + _b->v[13] * _a->v[4] + _b->v[14] * _a->v[8] + _b->v[15] * _a->v[12];
    ret.v[13] =
        _b->v[12] * _a->v[1] + _b->v[13] * _a->v[5] + _b->v[14] * _a->v[9] + _b->v[15] * _a->v[13];
    ret.v[14] =
        _b->v[12] * _a->v[2] + _b->v[13] * _a->v[6] + _b->v[14] * _a->v[10] + _b->v[15] * _a->v[14];
    ret.v[15] =
        _b->v[12] * _a->v[3] + _b->v[13] * _a->v[7] + _b->v[14] * _a->v[11] + _b->v[15] * _a->v[15];

    return ret;
}

TARP_API tpSegment
tpSegmentMake(tpFloat _h0x, tpFloat _h0y, tpFloat _px, tpFloat _py, tpFloat _h1x, tpFloat _h1y)
{
    tpSegment ret;
    ret.handleIn = tpVec2Make(_h0x, _h0y);
    ret.position = tpVec2Make(_px, _py);
    ret.handleOut = tpVec2Make(_h1x, _h1y);
    return ret;
}

TARP_API tpPaint tpPaintMakeColor(tpFloat _r, tpFloat _g, tpFloat _b, tpFloat _a)
{
    tpPaint ret;
    ret.data.color = tpColorMake(_r, _g, _b, _a);
    ret.type = kTpPaintTypeColor;
    return ret;
}

TARP_API tpPaint tpPaintMakeGradient(tpGradient _gradient)
{
    tpPaint ret;
    ret.data.gradient = _gradient;
    ret.type = kTpPaintTypeGradient;
    return ret;
}

#ifdef TARP_IMPLEMENTATION_OPENGL

/* @TODO: Clean up the layout of all the structs */

/* global to hold the last error message */
TARP_LOCAL TARP_TLS char __g_error[TARP_MAX_ERROR_MESSAGE];

TARP_LOCAL void _tpGLSetErrorMessage(const char * _message)
{
    strcpy(__g_error, _message);
}

/* The shader programs used by the renderer */
static const char * _vertexShaderCode =
    "#version 150 \n"
    "uniform mat4 transformProjection; \n"
    "uniform vec4 meshColor; \n"
    "in vec2 vertex; \n"
    "out vec4 icol;\n"
    "void main() \n"
    "{ \n"
    "gl_Position = transformProjection * vec4(vertex, 0.0, 1.0); \n"
    "icol = meshColor;\n"
    "} \n";

static const char * _fragmentShaderCode =
    "#version 150 \n"
    "in vec4 icol; \n"
    "out vec4 pixelColor; \n"
    "void main() \n"
    "{ \n"
    "pixelColor = icol; \n"
    "} \n";

static const char * _vertexShaderCodeTexture =
    "#version 150 \n"
    "uniform mat4 transformProjection; \n"
    "in vec2 vertex; \n"
    "in float tc; \n"
    "out float itc;\n"
    "void main() \n"
    "{ \n"
    "gl_Position = transformProjection * vec4(vertex, 0.0, 1.0); \n"
    "itc = tc; \n"
    "} \n";

static const char * _fragmentShaderCodeTexture =
    "#version 150 \n"
    "uniform sampler1D tex;\n"
    "in float itc; \n"
    "out vec4 pixelColor; \n"
    "void main() \n"
    "{ \n"
    "pixelColor = texture(tex, itc); \n"
    "} \n";

typedef struct _tpGLContext _tpGLContext;

typedef enum TARP_LOCAL
{
    /*
    The mask values are important! FillRaster needs to have at least
    2 bits. These need to be the lower bits in order to work with the
    Increment, Decrement stencil operations
    http://www.opengl.org/discussion_boards/showthread.php/149740-glStencilOp-s-GL_INCR-GL_DECR-behaviour-when-masked
    */
    _kTpGLFillRasterStencilPlane = 0x1F,    /* binary mask 00011111 */
    _kTpGLClippingStencilPlaneOne = 1 << 5, /* binary mask 00100000 */
    _kTpGLClippingStencilPlaneTwo = 1 << 6, /* binary mask 01000000 */
    _kTpGLStrokeRasterStencilPlane = 1 << 7 /* binary mask 10000000 */
} _tpGLStencilPlane;

typedef struct TARP_LOCAL
{
    tpVec2 min, max;
} _tpGLRect;

typedef struct TARP_LOCAL
{
    tpVec2 p0, h0, h1, p1;
} _tpGLCurve;

typedef struct TARP_LOCAL
{
    _tpGLCurve first, second;
} _tpGLCurvePair;

#define _TARP_ARRAY_T _tpFloatArray
#define _TARP_ITEM_T tpFloat
#include <Tarp/TarpArray.h>

#define _TARP_ARRAY_T _tpBoolArray
#define _TARP_ITEM_T tpBool
#include <Tarp/TarpArray.h>

#define _TARP_ARRAY_T _tpSegmentArray
#define _TARP_ITEM_T tpSegment
#define _TARP_COMPARATOR_T 0
#include <Tarp/TarpArray.h>

#define _TARP_ARRAY_T _tpVec2Array
#define _TARP_ITEM_T tpVec2
#define _TARP_COMPARATOR_T 0
#include <Tarp/TarpArray.h>

#define _TARP_ARRAY_T _tpColorStopArray
#define _TARP_ITEM_T tpColorStop
#define _TARP_COMPARATOR_T 0
#include <Tarp/TarpArray.h>

typedef struct TARP_LOCAL
{
    _tpSegmentArray segments;
    tpBool bDirty, bIsClosed, bLengthDirty;
    int lastSegmentIndex;

    /* some rendering specific data */
    int fillVertexOffset;
    int fillVertexCount;
    int strokeVertexOffset;
    int strokeVertexCount;

    _tpGLRect bounds;
    tpFloat length;
} _tpGLContour;

#define _TARP_ARRAY_T _tpGLContourArray
#define _TARP_ITEM_T _tpGLContour
#define _TARP_COMPARATOR_T 0
#include <Tarp/TarpArray.h>

typedef struct TARP_LOCAL
{
    tpVec2 vertex;
    tpVec2 tc;
} _tpGLTextureVertex;

#define _TARP_ARRAY_T _tpGLTextureVertexArray
#define _TARP_ITEM_T _tpGLTextureVertex
#define _TARP_COMPARATOR_T 0
#include <Tarp/TarpArray.h>

typedef struct TARP_LOCAL
{
    int gradientID;
    tpVec2 origin;
    tpVec2 destination;
    tpFloat ratio;
    tpVec2 focal_point_offset;
    _tpColorStopArray stops;
    tpGradientType type;

    /* rendering specific data/caches */
    tpBool bDirty;
    GLuint rampTexture;
} _tpGLGradient;

typedef struct TARP_LOCAL
{
    _tpGLRect * bounds; /* the bounds used by this gradient (i.e. stroke or fill) */
    int vertexOffset;
    int vertexCount;

} _tpGLGradientCacheData;

typedef struct TARP_LOCAL
{
    int fillVertexOffset;
    int fillVertexCount;
    int strokeVertexOffset;
    int strokeVertexCount;
    tpBool bIsClosed;
} _tpGLRenderCacheContour;

#define _TARP_ARRAY_T _tpGLRenderCacheContourArray
#define _TARP_ITEM_T _tpGLRenderCacheContour
#define _TARP_COMPARATOR_T 0
#include <Tarp/TarpArray.h>

typedef struct TARP_LOCAL
{
    _tpGLRenderCacheContourArray contours;
    _tpVec2Array geometryCache;
    _tpGLTextureVertexArray textureGeometryCache;
    _tpBoolArray jointCache;
    _tpGLRect boundsCache;
    _tpGLRect strokeBoundsCache;
    _tpGLGradientCacheData fillGradientData;
    _tpGLGradientCacheData strokeGradientData;
    int strokeVertexOffset;
    int strokeVertexCount;
    int boundsVertexOffset;
    tpMat4 renderMatrix; /* either the transform or transformProjection */
    tpStyle style;
    _tpFloatArray dashArrayStorage; /* used to potentially copy the dash array to that style uses */
} _tpGLRenderCache;

typedef struct TARP_LOCAL
{
    _tpGLContourArray contours;
    int currentContourIndex;

    tpBool bPathGeometryDirty;
    tpFloat lastTransformScale;

    int lastFillGradientID;
    int lastStrokeGradientID;

    _tpGLContext * lastDrawContext;
    int lastTransformID;

    tpTransform fillPaintTransform;
    tpTransform strokePaintTransform;
    tpBool bFillPaintTransformDirty;
    tpBool bStrokePaintTransformDirty;

    _tpGLRenderCache * renderCache;
} _tpGLPath;

typedef struct TARP_LOCAL
{
    GLuint vao;
    GLuint vbo;
    GLuint vboSize;
} _tpGLVAO;

typedef struct TARP_LOCAL
{
    GLenum activeTexture;
    GLboolean depthTest;
    GLboolean depthMask;
    GLboolean multisample;
    GLboolean stencilTest;
    GLuint stencilMask;
    GLenum stencilFail;
    GLenum stencilPassDepthPass;
    GLenum stencilPassDepthFail;
    GLuint clearStencil;
    GLboolean blending;
    GLenum blendSrcRGB;
    GLenum blendDestRGB;
    GLenum blendSrcAlpha;
    GLenum blendDestAlpha;
    GLenum blendEquationRGB;
    GLenum blendEquationAlpha;
    GLboolean cullFace;
    GLenum cullFaceMode;
    GLenum frontFace;
    GLuint vao;
    GLuint vbo;
    GLuint program;
} _tpGLStateBackup;

struct _tpGLContext
{
    GLuint program;
    GLuint textureProgram;
    GLuint tpLoc;
    GLuint tpTextureLoc;
    GLuint meshColorLoc;

    _tpGLVAO vao;
    _tpGLVAO textureVao;

    /* this array allocates render caches that the context takes ownership over in cases where a
     * render cache needs to be cloned */
    _tpGLRenderCache * clippingRenderCaches[TARP_GL_MAX_CLIPPING_STACK_DEPTH];
    /* this render cache array is the actual active clipping stack */
    _tpGLRenderCache * clippingStack[TARP_GL_MAX_CLIPPING_STACK_DEPTH];
    int clippingStackDepth;

    int currentClipStencilPlane;
    tpBool bCanSwapStencilPlanes;
    tpTransform transform;
    tpMat4 renderTransform;
    tpMat4 projection;
    tpMat4 transformProjection;
    tpFloat transformScale;
    int transformID;
    int projectionID;
    tpBool bTransformProjDirty;
    tpStyle clippingStyle;

    /* used to temporarily store vertex/stroke data (think double buffering)
     */
    _tpVec2Array tmpVertices;
    _tpBoolArray tmpJoints;
    _tpGLTextureVertexArray tmpTexVertices;
    _tpColorStopArray tmpColorStops;
    _tpGLRenderCacheContourArray tmpRcContours;

    _tpGLStateBackup stateBackup;
};

typedef struct TARP_LOCAL
{
    char message[TARP_GL_ERROR_MESSAGE_SIZE];
    int length;
} _ErrorMessage;

TARP_LOCAL void _tpGLGradientCacheDataInit(_tpGLGradientCacheData * _gd, _tpGLRect * _bounds)
{
    _gd->bounds = _bounds;
    _gd->vertexOffset = 0;
    _gd->vertexCount = 0;
}

/* @TODO: Get rid of all the _ErrorMessage things and call
 * _tpGLSetErrorMessage instead? */
TARP_LOCAL tpBool _compileShader(const char * _shaderCode,
                                 GLenum _shaderType,
                                 GLuint * _outHandle,
                                 _ErrorMessage * _outError)
{
    GLenum glHandle;
    GLint state, len, infologLength;

    glHandle = glCreateShader(_shaderType);
    len = (GLint)strlen(_shaderCode);
    _TARP_ASSERT_NO_GL_ERROR(glShaderSource(glHandle, 1, &_shaderCode, &len));
    _TARP_ASSERT_NO_GL_ERROR(glCompileShader(glHandle));

    /* check if the shader compiled */
    _TARP_ASSERT_NO_GL_ERROR(glGetShaderiv(glHandle, GL_COMPILE_STATUS, &state));
    if (state == GL_FALSE)
    {
        if (_outError)
        {
            _TARP_ASSERT_NO_GL_ERROR(glGetShaderiv(glHandle, GL_INFO_LOG_LENGTH, &infologLength));
            infologLength = TARP_MIN(TARP_GL_ERROR_MESSAGE_SIZE, infologLength);
            _TARP_ASSERT_NO_GL_ERROR(
                glGetShaderInfoLog(glHandle, infologLength, &infologLength, _outError->message));
            _outError->length = infologLength;
        }

        glDeleteShader(glHandle);
        return tpTrue;
    }
    else
    {
        *_outHandle = glHandle;
    }
    return tpFalse;
}

TARP_LOCAL tpBool _createProgram(const char * _vertexShader,
                                 const char * _fragmentShader,
                                 int _bTexProgram,
                                 GLuint * _outHandle,
                                 _ErrorMessage * _outError)
{
    GLuint vertexShader, fragmentShader, program;
    GLint state, infologLength;
    tpBool err;

    err = _compileShader(_vertexShader, GL_VERTEX_SHADER, &vertexShader, _outError);
    if (err)
        return err;

    err = _compileShader(_fragmentShader, GL_FRAGMENT_SHADER, &fragmentShader, _outError);
    if (err)
        return err;

    program = glCreateProgram();
    _TARP_ASSERT_NO_GL_ERROR(glAttachShader(program, vertexShader));
    _TARP_ASSERT_NO_GL_ERROR(glAttachShader(program, fragmentShader));

    _TARP_ASSERT_NO_GL_ERROR(glBindAttribLocation(program, 0, "vertex"));
    if (_bTexProgram)
        _TARP_ASSERT_NO_GL_ERROR(glBindAttribLocation(program, 1, "tc"));

    _TARP_ASSERT_NO_GL_ERROR(glLinkProgram(program));

    /* check if we had success */
    _TARP_ASSERT_NO_GL_ERROR(glGetProgramiv(program, GL_LINK_STATUS, &state));

    if (state == GL_FALSE)
    {
        if (_outError)
        {
            _TARP_ASSERT_NO_GL_ERROR(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLength));
            infologLength = TARP_MIN(TARP_GL_ERROR_MESSAGE_SIZE, infologLength);
            _TARP_ASSERT_NO_GL_ERROR(
                glGetProgramInfoLog(program, infologLength, &infologLength, _outError->message));
            _outError->length = infologLength;
        }

        return tpTrue;
    }

    _TARP_ASSERT_NO_GL_ERROR(glDeleteShader(vertexShader));
    _TARP_ASSERT_NO_GL_ERROR(glDeleteShader(fragmentShader));

    if (err)
    {
        glDeleteProgram(program);
    }
    else
    {
        *_outHandle = program;
    }

    return tpFalse;
}

TARP_API const char * tpErrorMessage()
{
    return __g_error;
}

TARP_API const char * tpImplementationName()
{
    return "OpenGL";
}

TARP_API tpRenderCache tpRenderCacheCreate()
{
    tpRenderCache ret;

    _tpGLRenderCache * renderCache = (_tpGLRenderCache *)TARP_MALLOC(sizeof(_tpGLRenderCache));

    _tpGLRenderCacheContourArrayInit(&renderCache->contours, 4);
    _tpVec2ArrayInit(&renderCache->geometryCache, 128);
    _tpGLTextureVertexArrayInit(&renderCache->textureGeometryCache, 8);
    _tpBoolArrayInit(&renderCache->jointCache, 128);

    _tpGLGradientCacheDataInit(&renderCache->fillGradientData, &renderCache->boundsCache);
    _tpGLGradientCacheDataInit(&renderCache->strokeGradientData, &renderCache->strokeBoundsCache);

    _tpFloatArrayInit(&renderCache->dashArrayStorage, 2);

    renderCache->strokeVertexOffset = 0;
    renderCache->strokeVertexCount = 0;
    renderCache->boundsVertexOffset = 0;

    ret.pointer = renderCache;
    return ret;
}

TARP_LOCAL void _tpGLRenderCacheClear(_tpGLRenderCache * _cache)
{
    _tpGLRenderCacheContourArrayClear(&_cache->contours);
    _tpVec2ArrayClear(&_cache->geometryCache);
    _tpGLTextureVertexArrayClear(&_cache->textureGeometryCache);
    _tpBoolArrayClear(&_cache->jointCache);
    _tpFloatArrayClear(&_cache->dashArrayStorage);
    _cache->strokeVertexOffset = 0;
    _cache->strokeVertexCount = 0;
    _cache->boundsVertexOffset = 0;
}

TARP_LOCAL void _tpGLRenderCacheCopyStyle(const tpStyle * _style, _tpGLRenderCache * _to)
{
    _to->style = *_style;
    /* if there is a dash array, we create a copy and make the _to style point to it */
    if (_style->dashCount)
    {
        _tpFloatArrayClear(&_to->dashArrayStorage);
        _tpFloatArrayAppendCArray(&_to->dashArrayStorage, _style->dashArray, _style->dashCount);
        _to->style.dashArray = _to->dashArrayStorage.array;
    }
}

TARP_LOCAL void _tpGLRenderCacheCopyTo(const _tpGLRenderCache * _from, _tpGLRenderCache * _to)
{
    _tpGLRenderCacheContourArrayClear(&_to->contours);
    _tpGLRenderCacheContourArrayAppendArray(&_to->contours, &_from->contours);
    _tpVec2ArrayClear(&_to->geometryCache);
    _tpVec2ArrayAppendArray(&_to->geometryCache, &_from->geometryCache);
    _tpGLTextureVertexArrayClear(&_to->textureGeometryCache);
    _tpGLTextureVertexArrayAppendArray(&_to->textureGeometryCache, &_from->textureGeometryCache);
    _tpBoolArrayClear(&_to->jointCache);
    _tpBoolArrayAppendArray(&_to->jointCache, &_from->jointCache);
    _to->boundsCache = _from->boundsCache;
    _to->strokeBoundsCache = _from->strokeBoundsCache;
    _to->fillGradientData.vertexOffset = _from->fillGradientData.vertexOffset;
    _to->fillGradientData.vertexCount = _from->fillGradientData.vertexCount;
    _to->strokeGradientData.vertexOffset = _from->strokeGradientData.vertexOffset;
    _to->strokeGradientData.vertexCount = _from->strokeGradientData.vertexCount;
    _to->strokeVertexOffset = _from->strokeVertexCount;
    _to->strokeVertexCount = _from->strokeVertexCount;
    _to->boundsVertexOffset = _from->boundsVertexOffset;
    _to->renderMatrix = _from->renderMatrix;
    _tpGLRenderCacheCopyStyle(&_from->style, _to);
}

TARP_LOCAL void _tpGLRenderCacheDestroyImpl(_tpGLRenderCache * _cache)
{
    if (_cache)
    {
        _tpBoolArrayDeallocate(&_cache->jointCache);
        _tpGLTextureVertexArrayDeallocate(&_cache->textureGeometryCache);
        _tpVec2ArrayDeallocate(&_cache->geometryCache);
        _tpGLRenderCacheContourArrayDeallocate(&_cache->contours);
        _tpFloatArrayDeallocate(&_cache->dashArrayStorage);
        TARP_FREE(_cache);
    }
}

TARP_API tpContext tpContextCreate()
{
    _ErrorMessage msg;
    _tpGLContext * ctx;
    tpBool err;
    int i;
    tpContext ret = tpContextInvalidHandle();

    ctx = (_tpGLContext *)TARP_MALLOC(sizeof(_tpGLContext));
    assert(ctx);

    err = _createProgram(_vertexShaderCode, _fragmentShaderCode, 0, &ctx->program, &msg);
    if (err)
    {
        _tpGLSetErrorMessage(msg.message);
        return ret;
    }
    err = _createProgram(
        _vertexShaderCodeTexture, _fragmentShaderCodeTexture, 1, &ctx->textureProgram, &msg);
    if (err)
    {
        _tpGLSetErrorMessage(msg.message);
        return ret;
    }

    ctx->tpLoc = glGetUniformLocation(ctx->program, "transformProjection");
    ctx->meshColorLoc = glGetUniformLocation(ctx->program, "meshColor");
    ctx->tpTextureLoc = glGetUniformLocation(ctx->textureProgram, "transformProjection");

    _TARP_ASSERT_NO_GL_ERROR(glGenVertexArrays(1, &ctx->vao.vao));
    _TARP_ASSERT_NO_GL_ERROR(glBindVertexArray(ctx->vao.vao));
    _TARP_ASSERT_NO_GL_ERROR(glGenBuffers(1, &ctx->vao.vbo));
    ctx->vao.vboSize = 0;
    _TARP_ASSERT_NO_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, ctx->vao.vbo));
    _TARP_ASSERT_NO_GL_ERROR(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, ((char *)0)));
    _TARP_ASSERT_NO_GL_ERROR(glEnableVertexAttribArray(0));

    _TARP_ASSERT_NO_GL_ERROR(glGenVertexArrays(1, &ctx->textureVao.vao));
    _TARP_ASSERT_NO_GL_ERROR(glBindVertexArray(ctx->textureVao.vao));
    _TARP_ASSERT_NO_GL_ERROR(glGenBuffers(1, &ctx->textureVao.vbo));
    ctx->textureVao.vboSize = 0;
    _TARP_ASSERT_NO_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, ctx->textureVao.vbo));
    _TARP_ASSERT_NO_GL_ERROR(
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(tpFloat), ((char *)0)));
    _TARP_ASSERT_NO_GL_ERROR(glEnableVertexAttribArray(0));
    _TARP_ASSERT_NO_GL_ERROR(glVertexAttribPointer(
        1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(tpFloat), ((char *)(2 * sizeof(float)))));
    _TARP_ASSERT_NO_GL_ERROR(glEnableVertexAttribArray(1));

    for (i = 0; i < TARP_GL_MAX_CLIPPING_STACK_DEPTH; ++i)
    {
        ctx->clippingRenderCaches[i] = (_tpGLRenderCache *)tpRenderCacheCreate().pointer;
    }

    ctx->clippingStackDepth = 0;
    ctx->currentClipStencilPlane = _kTpGLClippingStencilPlaneOne;
    ctx->bCanSwapStencilPlanes = tpTrue;
    ctx->transform = tpTransformMakeIdentity();
    ctx->renderTransform = tpMat4MakeIdentity();
    ctx->transformScale = 1.0;
    ctx->transformID = 0;
    ctx->projection = tpMat4MakeIdentity();
    ctx->projectionID = 0;
    ctx->bTransformProjDirty = tpFalse;
    ctx->transformProjection = tpMat4MakeIdentity();

    _tpVec2ArrayInit(&ctx->tmpVertices, 512);
    _tpBoolArrayInit(&ctx->tmpJoints, 256);
    _tpGLTextureVertexArrayInit(&ctx->tmpTexVertices, 64);
    _tpColorStopArrayInit(&ctx->tmpColorStops, 16);
    _tpGLRenderCacheContourArrayInit(&ctx->tmpRcContours, 16);

    ctx->clippingStyle = tpStyleMake();
    ctx->clippingStyle.stroke.type = kTpPaintTypeNone;

    ret.pointer = ctx;
    return ret;
}

TARP_API void tpContextDestroy(tpContext _ctx)
{
    int i;
    _tpGLContext * ctx = (_tpGLContext *)_ctx.pointer;

    /* free all opengl resources */
    glDeleteProgram(ctx->program);
    glDeleteBuffers(1, &ctx->vao.vbo);
    glDeleteVertexArrays(1, &ctx->vao.vao);
    glDeleteProgram(ctx->textureProgram);
    glDeleteBuffers(1, &ctx->textureVao.vbo);
    glDeleteVertexArrays(1, &ctx->textureVao.vao);

    _tpBoolArrayDeallocate(&ctx->tmpJoints);
    _tpVec2ArrayDeallocate(&ctx->tmpVertices);
    _tpGLTextureVertexArrayDeallocate(&ctx->tmpTexVertices);
    _tpColorStopArrayDeallocate(&ctx->tmpColorStops);
    _tpGLRenderCacheContourArrayDeallocate(&ctx->tmpRcContours);

    for (i = 0; i < TARP_GL_MAX_CLIPPING_STACK_DEPTH; ++i)
    {
        _tpGLRenderCacheDestroyImpl(ctx->clippingRenderCaches[i]);
    }

    TARP_FREE(ctx);
}

TARP_API void tpRenderCacheDestroy(tpRenderCache _cache)
{
    _tpGLRenderCacheDestroyImpl((_tpGLRenderCache *)_cache.pointer);
}

TARP_LOCAL _tpGLRenderCache * _tpGLPathEnsureRenderCache(_tpGLPath * _path)
{
    if (!_path->renderCache)
        _path->renderCache = (_tpGLRenderCache *)tpRenderCacheCreate().pointer;

    return _path->renderCache;
}

TARP_API tpPath tpPathCreate()
{
    tpPath ret;

    _tpGLPath * path = (_tpGLPath *)TARP_MALLOC(sizeof(_tpGLPath));
    _tpGLContourArrayInit(&path->contours, 4);
    path->currentContourIndex = -1;

    path->bPathGeometryDirty = tpTrue;
    path->lastTransformScale = 1.0;

    path->lastDrawContext = NULL;
    path->lastTransformID = 0;

    path->lastFillGradientID = -1;
    path->lastStrokeGradientID = -1;

    path->fillPaintTransform = tpTransformMakeIdentity();
    path->strokePaintTransform = tpTransformMakeIdentity();
    path->bFillPaintTransformDirty = tpFalse;
    path->bStrokePaintTransformDirty = tpFalse;

    path->renderCache = NULL;

    ret.pointer = path;
    return ret;
}

TARP_API tpPath tpPathClone(tpPath _path)
{
    int i;
    tpPath ret;
    _tpGLPath *from, *path;

    if (!tpPathIsValidHandle(_path))
        return tpPathInvalidHandle();

    ret = tpPathCreate();

    from = (_tpGLPath *)_path.pointer;
    path = (_tpGLPath *)ret.pointer;

    if (from->contours.count)
    {
        _tpGLContourArrayReserve(&path->contours, from->contours.count);
        for (i = 0; i < from->contours.count; ++i)
        {
            _tpGLContour contour;
            _tpGLContour * fromCont = _tpGLContourArrayAtPtr(&from->contours, i);
            _tpSegmentArrayInit(&contour.segments, fromCont->segments.count);
            _tpSegmentArrayAppendArray(&contour.segments, &fromCont->segments);
            contour.segments = _tpSegmentArrayCopy(&fromCont->segments);
            contour.bDirty = fromCont->bDirty;
            contour.bIsClosed = fromCont->bIsClosed;
            contour.bLengthDirty = fromCont->bLengthDirty;
            contour.fillVertexOffset = fromCont->fillVertexOffset;
            contour.fillVertexCount = fromCont->fillVertexCount;
            contour.strokeVertexOffset = fromCont->strokeVertexOffset;
            contour.strokeVertexCount = fromCont->strokeVertexCount;
            contour.bounds = fromCont->bounds;
            contour.length = fromCont->length;
            _tpGLContourArrayAppendPtr(&path->contours, &contour);
        }
    }

    path->currentContourIndex = from->currentContourIndex;

    path->bPathGeometryDirty = from->bPathGeometryDirty;
    path->lastTransformScale = from->lastTransformScale;

    path->fillPaintTransform = from->fillPaintTransform;
    path->strokePaintTransform = from->strokePaintTransform;
    path->bFillPaintTransformDirty = from->bFillPaintTransformDirty;
    path->bStrokePaintTransformDirty = from->bStrokePaintTransformDirty;

    path->lastDrawContext = from->lastDrawContext;
    path->lastTransformID = from->lastTransformID;

    return ret;
}

TARP_API void tpPathDestroy(tpPath _path)
{
    int i;
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    if (p)
    {
        for (i = 0; i < p->contours.count; ++i)
        {
            _tpSegmentArrayDeallocate(&_tpGLContourArrayAtPtr(&p->contours, i)->segments);
        }
        _tpGLContourArrayDeallocate(&p->contours);
        _tpGLRenderCacheDestroyImpl(p->renderCache);

        TARP_FREE(p);
    }
}

TARP_LOCAL _tpGLContour * _tpGLCurrentContour(_tpGLPath * _p)
{
    if (_p->currentContourIndex != -1)
        return _tpGLContourArrayAtPtr(&_p->contours, _p->currentContourIndex);
    return NULL;
}

TARP_LOCAL _tpGLContour * _tpGLPathCreateNextContour(_tpGLPath * _p)
{
    _tpGLContour contour;
    _tpSegmentArrayInit(&contour.segments, 8);
    contour.bDirty = tpTrue;
    contour.lastSegmentIndex = -1;
    contour.bIsClosed = tpFalse;
    contour.fillVertexOffset = 0;
    contour.fillVertexCount = 0;
    contour.strokeVertexOffset = 0;
    contour.strokeVertexCount = 0;
    contour.bLengthDirty = tpTrue;
    _p->currentContourIndex = _p->contours.count;
    _tpGLContourArrayAppendPtr(&_p->contours, &contour);
    return _tpGLContourArrayAtPtr(&_p->contours, _p->currentContourIndex);
}

TARP_LOCAL _tpGLContour * _tpGLPathNextEmptyContour(_tpGLPath * _path)
{
    _tpGLContour * c = _tpGLCurrentContour(_path);
    if (!c || c->segments.count)
    {
        c = _tpGLPathCreateNextContour(_path);
    }
    return c;
}

TARP_LOCAL tpBool _tpGLContourAddSegments(_tpGLPath * _p,
                                          _tpGLContour * _c,
                                          const tpSegment * _segments,
                                          int _count)
{
    int err = _tpSegmentArrayAppendCArray(&_c->segments, _segments, _count);
    if (err)
    {
        _tpGLSetErrorMessage("Could not allocate memory for segments.");
        return tpTrue;
    }

    _c->lastSegmentIndex = _c->segments.count - 1;
    _c->bDirty = tpTrue;
    _p->bPathGeometryDirty = tpTrue;

    return tpFalse;
}

TARP_LOCAL tpBool _tpGLContourAddSegment(_tpGLPath * _p,
                                         _tpGLContour * _c,
                                         tpFloat _h0x,
                                         tpFloat _h0y,
                                         tpFloat _px,
                                         tpFloat _py,
                                         tpFloat _h1x,
                                         tpFloat _h1y)
{
    tpSegment seg = tpSegmentMake(_h0x, _h0y, _px, _py, _h1x, _h1y);
    return _tpGLContourAddSegments(_p, _c, &seg, 1);
}

TARP_API tpBool tpPathAddSegment(
    tpPath _path, tpFloat _h0x, tpFloat _h0y, tpFloat _px, tpFloat _py, tpFloat _h1x, tpFloat _h1y)
{
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    _tpGLContour * c = _tpGLCurrentContour(p);
    if (!c)
    {
        c = _tpGLPathCreateNextContour(p);
        assert(c);
    }

    if (!c)
        return tpTrue;

    return _tpGLContourAddSegment(p, c, _h0x, _h0y, _px, _py, _h1x, _h1y);
}

TARP_API tpBool tpPathLineTo(tpPath _path, tpFloat _x, tpFloat _y)
{
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    _tpGLContour * c = _tpGLCurrentContour(p);
    if (!c || c->lastSegmentIndex == -1)
    {
        _tpGLSetErrorMessage(
            "You have to start a contour before issuing "
            "this command (see tpPathMoveTo).");
        return tpTrue;
    }

    return _tpGLContourAddSegment(p, c, _x, _y, _x, _y, _x, _y);
}

TARP_API tpBool tpPathMoveTo(tpPath _path, tpFloat _x, tpFloat _y)
{
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    _tpGLContour * c = _tpGLPathNextEmptyContour(p);

    return _tpGLContourAddSegment(p, c, _x, _y, _x, _y, _x, _y);
}

TARP_API tpBool tpPathClear(tpPath _path)
{
    int i;
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    p->currentContourIndex = -1;
    for (i = 0; i < p->contours.count; ++i)
    {
        _tpSegmentArrayDeallocate(&_tpGLContourArrayAtPtr(&p->contours, i)->segments);
    }
    _tpGLContourArrayClear(&p->contours);
    p->bPathGeometryDirty = tpTrue;

    return tpFalse;
}

TARP_API int tpPathContourCount(tpPath _path)
{
    return ((_tpGLPath *)_path.pointer)->contours.count;
}

TARP_API tpBool tpPathCubicCurveTo(
    tpPath _path, tpFloat _h0x, tpFloat _h0y, tpFloat _h1x, tpFloat _h1y, tpFloat _px, tpFloat _py)
{
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    _tpGLContour * c = _tpGLCurrentContour(p);
    if (!c || c->lastSegmentIndex == -1)
    {
        _tpGLSetErrorMessage(
            "You have to start a contour before issuing "
            "this command (see tpPathMoveTo).");
        return tpTrue;
    }

    _tpSegmentArrayAtPtr(&c->segments, c->lastSegmentIndex)->handleOut = tpVec2Make(_h0x, _h0y);
    return _tpGLContourAddSegment(p, c, _h1x, _h1y, _px, _py, _px, _py);
}

TARP_API tpBool
tpPathQuadraticCurveTo(tpPath _path, tpFloat _hx, tpFloat _hy, tpFloat _px, tpFloat _py)
{
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    _tpGLContour * c = _tpGLCurrentContour(p);
    if (!c || c->lastSegmentIndex == -1)
    {
        _tpGLSetErrorMessage(
            "You have to start a contour before issuing "
            "this command (see tpPathMoveTo).");
        return tpTrue;
    }

    _tpSegmentArrayAtPtr(&c->segments, c->lastSegmentIndex)->handleOut = tpVec2Make(_hx, _hy);
    return _tpGLContourAddSegment(p, c, _hx, _hy, _px, _py, _px, _py);
}
    
// Arc calculation code based on nanosvg (https://github.com/memononen/nanosvg)
// itself based on canvg (https://code.google.com/p/canvg/)
static float nsvg__sqr(float x) { return x*x; }
static float nsvg__vmag(float x, float y) { return sqrtf(x*x + y*y); }
static float nsvg__vecrat(float ux, float uy, float vx, float vy)
{
    return (ux*vx + uy*vy) / (nsvg__vmag(ux,uy) * nsvg__vmag(vx,vy));
}
static float nsvg__vecang(float ux, float uy, float vx, float vy)
{
    float r = nsvg__vecrat(ux,uy, vx,vy);
    if (r < -1.0f) r = -1.0f;
    if (r > 1.0f) r = 1.0f;
    return ((ux*vy < uy*vx) ? -1.0f : 1.0f) * acosf(r);
}
static void nsvg__xformPoint(float* dx, float* dy, float x, float y, float* t)
{
    *dx = x*t[0] + y*t[2] + t[4];
    *dy = x*t[1] + y*t[3] + t[5];
}
static void nsvg__xformVec(float* dx, float* dy, float x, float y, float* t)
{
    *dx = x*t[0] + y*t[2];
    *dy = x*t[1] + y*t[3];
}

TARP_API tpBool
tpPathArcTo(tpPath _path, tpFloat _rx, tpFloat _ry, tpFloat _rotx, tpFloat _fl, tpFloat _swfl, tpFloat _x, tpFloat _y)
{
    float rx, ry, rotx;
    float x1, y1, x2, y2, cx, cy, dx, dy, d;
    float x1p, y1p, cxp, cyp, s, sa, sb;
    float ux, uy, vx, vy, a1, da;
    float x, y, tanx, tany, a, px = 0, py = 0, ptanx = 0, ptany = 0, t[6];
    float sinrx, cosrx;
    int fa, fs;
    int i, ndivs;
    float hda, kappa;

    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    _tpGLContour * c = _tpGLCurrentContour(p);
    if (!c || c->lastSegmentIndex == -1)
    {
        _tpGLSetErrorMessage(
            "You have to start a contour before issuing "
            "this command (see tpPathArcTo).");
        return tpTrue;
    }

    rx = _rx;                // y radius
    ry = _ry;                // x radius
    rotx = _rotx / 180.0f * 3.14;      // x rotation angle
    fa = fabsf(_fl) > 1e-6 ? 1 : 0; // Large arc
    fs = fabsf(_swfl) > 1e-6 ? 1 : 0; // Sweep direction
    x1 = _tpSegmentArrayAtPtr(&c->segments, c->lastSegmentIndex)->position.x;                          // start point
    y1 = _tpSegmentArrayAtPtr(&c->segments, c->lastSegmentIndex)->position.y;
    x2 = _x;
    y2 = _y;

    dx = x1 - x2;
    dy = y1 - y2;
    d = sqrtf(dx*dx + dy*dy);
    if (d < 1e-6f || rx < 1e-6f || ry < 1e-6f) {
        // The arc degenerates to a line
        return tpPathLineTo(_path, x2, y2);
    }

    sinrx = sinf(rotx);
    cosrx = cosf(rotx);

    // Convert to center point parameterization.
    // http://www.w3.org/TR/SVG11/implnote.html#ArcImplementationNotes
    // 1) Compute x1', y1'
    x1p = cosrx * dx / 2.0f + sinrx * dy / 2.0f;
    y1p = -sinrx * dx / 2.0f + cosrx * dy / 2.0f;
    d = nsvg__sqr(x1p)/nsvg__sqr(rx) + nsvg__sqr(y1p)/nsvg__sqr(ry);
    if (d > 1) {
        d = sqrtf(d);
        rx *= d;
        ry *= d;
    }
    // 2) Compute cx', cy'
    s = 0.0f;
    sa = nsvg__sqr(rx)*nsvg__sqr(ry) - nsvg__sqr(rx)*nsvg__sqr(y1p) - nsvg__sqr(ry)*nsvg__sqr(x1p);
    sb = nsvg__sqr(rx)*nsvg__sqr(y1p) + nsvg__sqr(ry)*nsvg__sqr(x1p);
    if (sa < 0.0f) sa = 0.0f;
    if (sb > 0.0f)
        s = sqrtf(sa / sb);
    if (fa == fs)
        s = -s;
    cxp = s * rx * y1p / ry;
    cyp = s * -ry * x1p / rx;

    // 3) Compute cx,cy from cx',cy'
    cx = (x1 + x2)/2.0f + cosrx*cxp - sinrx*cyp;
    cy = (y1 + y2)/2.0f + sinrx*cxp + cosrx*cyp;

    // 4) Calculate theta1, and delta theta.
    ux = (x1p - cxp) / rx;
    uy = (y1p - cyp) / ry;
    vx = (-x1p - cxp) / rx;
    vy = (-y1p - cyp) / ry;
    a1 = nsvg__vecang(1.0f,0.0f, ux,uy);    // Initial angle
    da = nsvg__vecang(ux,uy, vx,vy);        // Delta angle

//  if (vecrat(ux,uy,vx,vy) <= -1.0f) da = NSVG_PI;
//  if (vecrat(ux,uy,vx,vy) >= 1.0f) da = 0;

    if (fs == 0 && da > 0)
        da -= 2 * TARP_PI;
    else if (fs == 1 && da < 0)
        da += 2 * TARP_PI;

    // Approximate the arc using cubic spline segments.
    t[0] = cosrx; t[1] = sinrx;
    t[2] = -sinrx; t[3] = cosrx;
    t[4] = cx; t[5] = cy;

    // Split arc into max 90 degree segments.
    // The loop assumes an iteration per end point (including start and end), this +1.
    ndivs = (int)(fabsf(da) / (TARP_HALF_PI) + 1.0f);
    hda = (da / (float)ndivs) / 2.0f;
    kappa = fabsf(4.0f / 3.0f * (1.0f - cosf(hda)) / sinf(hda));
    if (da < 0.0f)
        kappa = -kappa;

    for (i = 0; i <= ndivs; i++) {
        a = a1 + da * ((float)i/(float)ndivs);
        dx = cosf(a);
        dy = sinf(a);
        nsvg__xformPoint(&x, &y, dx*rx, dy*ry, t); // position
        nsvg__xformVec(&tanx, &tany, -dy*rx * kappa, dx*ry * kappa, t); // tangent

        if (i > 0) {
            tpBool ret = tpPathCubicCurveTo(_path, px+ptanx,py+ptany, x-tanx, y-tany, x, y);
            if (ret) return ret;
        }
        px = x;
        py = y;
        ptanx = tanx;
        ptany = tany;
    }

    //*cpx = x2;
    //*cpy = y2;
    return tpFalse;
}
    
TARP_API tpBool tpPathClose(tpPath _path)
{
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    _tpGLContour * c = _tpGLCurrentContour(p);
    if (c && c->segments.count > 1)
    {
        c->bIsClosed = tpTrue;
        c->bDirty = tpTrue;
        p->currentContourIndex = -1;
        p->bPathGeometryDirty = tpTrue;
    }
    else
    {
        _tpGLSetErrorMessage(
            "tpPathClose failed because the path has no "
            "contour or the current contour is empty.");
    }
    return tpFalse;
}

TARP_API tpBool tpPathRemoveContour(tpPath _path, int _index)
{
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    _tpGLContour * c = _tpGLContourArrayAtPtr(&p->contours, _index);
    _tpSegmentArrayDeallocate(&c->segments);
    _tpGLContourArrayRemove(&p->contours, _index);
    p->bPathGeometryDirty = tpTrue;
    p->currentContourIndex = p->contours.count - 1;
    return tpFalse;
}

TARP_API tpBool tpPathRemoveSegment(tpPath _path, int _contourIndex, int _index)
{
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    _tpGLContour * c = _tpGLContourArrayAtPtr(&p->contours, _contourIndex);
    _tpSegmentArrayRemove(&c->segments, _index);
    p->bPathGeometryDirty = tpTrue;
    c->bDirty = tpTrue;
    return tpFalse;
}

TARP_API tpBool tpPathRemoveSegments(tpPath _path, int _contourIndex, int _from, int _to)
{
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    _tpGLContour * c = _tpGLContourArrayAtPtr(&p->contours, _contourIndex);
    _tpSegmentArrayRemoveRange(&c->segments, _from, _to);
    p->bPathGeometryDirty = tpTrue;
    c->bDirty = tpTrue;
    return tpFalse;
}

TARP_LOCAL tpBool _tpGLPathAddSegmentsToCurrentContour(_tpGLPath * _p,
                                                       _tpGLContour * _c,
                                                       const tpSegment * _segments,
                                                       int _count)
{
    int err = _tpSegmentArrayAppendCArray(&_c->segments, _segments, _count);
    if (err)
    {
        _tpGLSetErrorMessage("Could not allocate memory for segments.");
        return tpTrue;
    }

    _c->lastSegmentIndex = _c->segments.count - 1;
    _c->bDirty = tpTrue;
    _p->bPathGeometryDirty = tpTrue;
    return tpFalse;
}

TARP_API tpBool tpPathAddSegments(tpPath _path, const tpSegment * _segments, int _count)
{
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    _tpGLContour * c = _tpGLCurrentContour(p);
    if (!c)
    {
        c = _tpGLPathCreateNextContour(p);
        assert(c);
    }

    return _tpGLPathAddSegmentsToCurrentContour(p, c, _segments, _count);
}

TARP_API tpBool tpPathAddContour(tpPath _path,
                                 const tpSegment * _segments,
                                 int _count,
                                 tpBool _bClosed)
{
    tpBool ret;
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    _tpGLContour * c = _tpGLPathCreateNextContour(p);
    assert(c);

    ret = _tpGLPathAddSegmentsToCurrentContour(p, c, _segments, _count);
    if (ret)
        return ret;

    if (_bClosed)
        return tpPathClose(_path);
    return tpFalse;
}

TARP_API tpBool tpPathSetContour(
    tpPath _path, int _contourIndex, const tpSegment * _segments, int _count, tpBool _bClosed)
{
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    if (_contourIndex < p->contours.count)
    {
        _tpGLContour * c = _tpGLContourArrayAtPtr(&p->contours, _contourIndex);
        _tpSegmentArrayClear(&c->segments);
        _tpSegmentArrayAppendCArray(&c->segments, _segments, _count);
        c->lastSegmentIndex = c->segments.count - 1;
        c->bIsClosed = tpTrue;
        c->bDirty = tpTrue;
        p->bPathGeometryDirty = tpTrue;
        return tpFalse;
    }
    else
        return tpPathAddContour(_path, _segments, _count, _bClosed);
}

TARP_API tpBool tpPathAddCircle(tpPath _path, tpFloat _x, tpFloat _y, tpFloat _r)
{
    tpFloat dr = _r * 2.0;
    return tpPathAddEllipse(_path, _x, _y, dr, dr);
}

TARP_API tpBool
tpPathAddEllipse(tpPath _path, tpFloat _x, tpFloat _y, tpFloat _width, tpFloat _height)
{
    static tpVec2 s_unitSegments[12] = { { 1, 0 },  { 0, -TARP_KAPPA }, { 0, TARP_KAPPA },
                                         { 0, 1 },  { TARP_KAPPA, 0 },  { -TARP_KAPPA, 0 },
                                         { -1, 0 }, { 0, TARP_KAPPA },  { 0, -TARP_KAPPA },
                                         { 0, -1 }, { -TARP_KAPPA, 0 }, { TARP_KAPPA, 0 } };

    int i;
    tpFloat rw, rh, px, py;
    tpSegment segs[4];
    tpBool err;

    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    _tpGLContour * c = _tpGLPathNextEmptyContour(p);

    rw = _width * 0.5;
    rh = _height * 0.5;
    for (i = 0; i < 4; ++i)
    {
        int kappaIdx = i * 3;

        px = s_unitSegments[kappaIdx].x * rw + _x;
        py = s_unitSegments[kappaIdx].y * rh + _y;
        segs[i] = tpSegmentMake(s_unitSegments[kappaIdx + 1].x * rw + px,
                                s_unitSegments[kappaIdx + 1].y * rh + py,
                                px,
                                py,
                                s_unitSegments[kappaIdx + 2].x * rw + px,
                                s_unitSegments[kappaIdx + 2].y * rh + py);
    }

    err = _tpGLContourAddSegments(p, c, segs, 4);
    if (err)
        return err;

    tpPathClose(_path);

    return tpFalse;
}

TARP_API tpBool tpPathAddRect(tpPath _path, tpFloat _x, tpFloat _y, tpFloat _width, tpFloat _height)
{
    tpSegment segs[4];
    tpFloat tmpa, tmpb;
    tpBool err;

    _tpGLPath * p = (_tpGLPath *)_path.pointer;

    _tpGLContour * c = _tpGLPathNextEmptyContour(p);

    tmpa = _x + _width;
    tmpb = _y + _height;
    segs[0] = tpSegmentMake(_x, _y, _x, _y, _x, _y);
    segs[1] = tpSegmentMake(tmpa, _y, tmpa, _y, tmpa, _y);
    segs[2] = tpSegmentMake(tmpa, tmpb, tmpa, tmpb, tmpa, tmpb);
    segs[3] = tpSegmentMake(_x, tmpb, _x, tmpb, _x, tmpb);

    err = _tpGLContourAddSegments(p, c, segs, 4);
    if (err)
        return err;

    tpPathClose(_path);

    return tpFalse;
}

TARP_LOCAL void _tpGLPathMarkAllContoursDirty(_tpGLPath * _p)
{
    int i;
    _tpGLContour * c;
    _p->bPathGeometryDirty = tpTrue;
    for (i = 0; i < _p->contours.count; ++i)
    {
        c = _tpGLContourArrayAtPtr(&_p->contours, i);
        c->bDirty = tpTrue;
        c->bLengthDirty = tpTrue;
    }
}

TARP_API tpBool tpPathSetFillPaintTransform(tpPath _path, const tpTransform * _transform)
{
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    p->fillPaintTransform = *_transform;
    p->bFillPaintTransformDirty = tpTrue;
    return tpFalse;
}

TARP_API tpBool tpPathSetStrokePaintTransform(tpPath _path, const tpTransform * _transform)
{
    _tpGLPath * p = (_tpGLPath *)_path.pointer;
    p->strokePaintTransform = *_transform;
    p->bStrokePaintTransformDirty = tpTrue;
    return tpFalse;
}

TARP_API tpStyle tpStyleMake()
{
    tpStyle ret;

    ret.fill.data.color = tpColorMake(1, 1, 1, 1);
    ret.fill.type = kTpPaintTypeColor;
    ret.stroke.data.color = tpColorMake(0, 0, 0, 1);
    ret.stroke.type = kTpPaintTypeColor;
    ret.strokeWidth = 1.0;
    ret.strokeJoin = kTpStrokeJoinBevel;
    ret.strokeCap = kTpStrokeCapButt;
    ret.fillRule = kTpFillRuleEvenOdd;
    ret.dashCount = 0;
    ret.dashOffset = 0;
    ret.miterLimit = 4;
    ret.scaleStroke = tpTrue;

    return ret;
}

TARP_LOCAL _tpGLGradient * tpGradientCreate()
{
    static int s_id = 0;

    _tpGLGradient * ret = (_tpGLGradient *)TARP_MALLOC(sizeof(_tpGLGradient));
    ret->bDirty = tpTrue;
    /*
    the static id and incrementing is not multi threadding friendly...no
    care for now thread local storage will most likely be the nicest way to
    make this thread safe
    */
    ret->gradientID = s_id++;

    _TARP_ASSERT_NO_GL_ERROR(glGenTextures(1, &ret->rampTexture));
    _TARP_ASSERT_NO_GL_ERROR(glBindTexture(GL_TEXTURE_1D, ret->rampTexture));
    _TARP_ASSERT_NO_GL_ERROR(glTexImage1D(
        GL_TEXTURE_1D, 0, GL_RGBA, TARP_GL_RAMP_TEXTURE_SIZE, 0, GL_RGBA, GL_FLOAT, NULL));
    _TARP_ASSERT_NO_GL_ERROR(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    _TARP_ASSERT_NO_GL_ERROR(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    _TARP_ASSERT_NO_GL_ERROR(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    _TARP_ASSERT_NO_GL_ERROR(glBindTexture(GL_TEXTURE_1D, 0));

    return ret;
}

TARP_API tpGradient tpGradientCreateLinear(tpFloat _x0, tpFloat _y0, tpFloat _x1, tpFloat _y1)
{
    tpGradient rh;
    _tpGLGradient * ret = tpGradientCreate();
    _tpColorStopArrayInit(&ret->stops, 8);
    ret->type = kTpGradientTypeLinear;
    ret->origin = tpVec2Make(_x0, _y0);
    ret->destination = tpVec2Make(_x1, _y1);
    rh.pointer = ret;
    return rh;
}

TARP_API tpGradient tpGradientCreateRadial(
    tpFloat _fx, tpFloat _fy, tpFloat _ox, tpFloat _oy, tpFloat _dx, tpFloat _dy, tpFloat _ratio)
{
    tpGradient rh;
    _tpGLGradient * ret = tpGradientCreate();
    _tpColorStopArrayInit(&ret->stops, 8);
    ret->type = kTpGradientTypeRadial;
    ret->origin = tpVec2Make(_ox, _oy);
    ret->destination = tpVec2Make(_dx, _dy);
    ret->ratio = _ratio;
    ret->focal_point_offset = tpVec2Make(_fx, _fy);
    rh.pointer = ret;
    return rh;
}

TARP_API tpGradient tpGradientCreateRadialSymmetric(tpFloat _x, tpFloat _y, tpFloat _radius)
{
    return tpGradientCreateRadial(0, 0, _x, _y, _x + _radius, _y, 1);
}

TARP_API tpGradient tpGradientClone(tpGradient _gradient)
{
    tpGradient rh;
    _tpGLGradient *ret, *grad;
    int id;

    if (!tpGradientIsValidHandle(_gradient))
        return tpGradientInvalidHandle();

    ret = tpGradientCreate();
    grad = (_tpGLGradient *)_gradient.pointer;
    id = ret->gradientID;

    *ret = *grad;
    ret->gradientID = id;
    _tpColorStopArrayInit(&ret->stops, grad->stops.count);
    _tpColorStopArrayAppendArray(&ret->stops, &grad->stops);

    rh.pointer = ret;
    return rh;
}

TARP_API void tpGradientSetPositions(
    tpGradient _gradient, tpFloat _x0, tpFloat _y0, tpFloat _x1, tpFloat _y1)
{
    _tpGLGradient * g = (_tpGLGradient *)_gradient.pointer;
    g->origin = tpVec2Make(_x0, _y0);
    g->destination = tpVec2Make(_x1, _y1);
    g->bDirty = tpTrue;
}

TARP_API void tpGradientSetFocalPointOffset(tpGradient _gradient, tpFloat _x, tpFloat _y)
{
    _tpGLGradient * g = (_tpGLGradient *)_gradient.pointer;
    g->focal_point_offset = tpVec2Make(_x, _y);
    g->bDirty = tpTrue;
}

TARP_API void tpGradientSetRatio(tpGradient _gradient, tpFloat _ratio)
{
    _tpGLGradient * g = (_tpGLGradient *)_gradient.pointer;
    /* since it only makes sense for radial grads, we assert so we can at
     * least catch it in debug */
    assert(g->type == kTpGradientTypeRadial);
    g->ratio = _ratio;
    g->bDirty = tpTrue;
}

TARP_API void tpGradientAddColorStop(
    tpGradient _gradient, tpFloat _r, tpFloat _g, tpFloat _b, tpFloat _a, tpFloat _offset)
{
    _tpGLGradient * g = (_tpGLGradient *)_gradient.pointer;
    tpColorStop stop;
    stop.color = tpColorMake(_r, _g, _b, _a);
    stop.offset = _offset;
    _tpColorStopArrayAppendPtr(&g->stops, &stop);
    g->bDirty = tpTrue;
}

TARP_API void tpGradientClearColorStops(tpGradient _gradient)
{
    _tpGLGradient * g = (_tpGLGradient *)_gradient.pointer;
    _tpColorStopArrayClear(&g->stops);
    g->bDirty = tpTrue;
}

TARP_API void tpGradientDestroy(tpGradient _gradient)
{
    _tpGLGradient * g = (_tpGLGradient *)_gradient.pointer;
    if (g)
    {
        _TARP_ASSERT_NO_GL_ERROR(glDeleteTextures(1, &g->rampTexture));
        _tpColorStopArrayDeallocate(&g->stops);
        TARP_FREE(g);
    }
}

TARP_LOCAL int _tpGLIsClose(tpFloat _x, tpFloat _y, tpFloat _epsilon)
{
    tpFloat maxXYOne = TARP_MAX(1.0, TARP_MAX(fabs(_x), fabs(_y)));
    return fabs(_x - _y) <= _epsilon * maxXYOne;
}

TARP_LOCAL int _tpGLIsCurveLinear(const _tpGLCurve * _curve)
{
    return _tpGLIsClose(_curve->p0.x, _curve->h0.x, FLT_EPSILON) &&
           _tpGLIsClose(_curve->p0.y, _curve->h0.y, FLT_EPSILON) &&
           _tpGLIsClose(_curve->p1.x, _curve->h1.x, FLT_EPSILON) &&
           _tpGLIsClose(_curve->p1.y, _curve->h1.y, FLT_EPSILON);
}

TARP_LOCAL int _tpGLIsCurveFlatEnough(const _tpGLCurve * _curve, tpFloat _tolerance)
{
    tpFloat ux, uy, vx, vy;
    if (_tpGLIsCurveLinear(_curve))
        return 1;

    /*
    Comment from paper.js source in Curve.js:
    Thanks to Kaspar Fischer and Roger Willcocks for the following:
    http://hcklbrrfnn.files.wordpress.com/2012/08/bez.pdf
    */
    ux = _curve->h0.x * 3.0 - _curve->p0.x * 2.0 - _curve->p1.x;
    uy = _curve->h0.y * 3.0 - _curve->p0.y * 2.0 - _curve->p1.y;
    vx = _curve->h1.x * 3.0 - _curve->p1.x * 2.0 - _curve->p0.x;
    vy = _curve->h1.y * 3.0 - _curve->p1.y * 2.0 - _curve->p0.y;

    return TARP_MAX(ux * ux, vx * vx) + TARP_MAX(uy * uy, vy * vy) < 10 * _tolerance * _tolerance;
}

TARP_LOCAL void _tpGLSubdivideCurve(const _tpGLCurve * _curve, tpFloat _t, _tpGLCurvePair * _result)
{
    tpVec2 p0h0, h0h1, h1p1, p0h0h1, h0h1p1, p0h0h1p1;

    p0h0 = tpVec2Lerp(_curve->p0, _curve->h0, _t);
    h0h1 = tpVec2Lerp(_curve->h0, _curve->h1, _t);
    h1p1 = tpVec2Lerp(_curve->h1, _curve->p1, _t);

    p0h0h1 = tpVec2Lerp(p0h0, h0h1, _t);
    h0h1p1 = tpVec2Lerp(h0h1, h1p1, _t);

    p0h0h1p1 = tpVec2Lerp(p0h0h1, h0h1p1, _t);

    _result->first.p0 = _curve->p0;
    _result->first.h0 = p0h0;
    _result->first.h1 = p0h0h1;
    _result->first.p1 = p0h0h1p1;

    _result->second.p0 = p0h0h1p1;
    _result->second.h0 = h0h1p1;
    _result->second.h1 = h1p1;
    _result->second.p1 = _curve->p1;
}

TARP_LOCAL void _tpGLEvaluatePointForBounds(tpVec2 _vec, _tpGLRect * _bounds)
{
    _bounds->min.x = TARP_MIN(_vec.x, _bounds->min.x);
    _bounds->min.y = TARP_MIN(_vec.y, _bounds->min.y);
    _bounds->max.x = TARP_MAX(_vec.x, _bounds->max.x);
    _bounds->max.y = TARP_MAX(_vec.y, _bounds->max.y);
}

tpFloat _tpGLShortestAngle(tpVec2 _d0, tpVec2 _d1)
{
    tpFloat theta = acos(_d0.x * _d1.x + _d0.y * _d1.y);

    /* make sure we have the shortest angle */
    if (theta > TARP_HALF_PI)
        theta = TARP_PI - theta;

    return theta;
}

TARP_LOCAL void _tpGLPushTriangle(_tpVec2Array * _vertices, tpVec2 _a, tpVec2 _b, tpVec2 _c)
{
    _tpVec2ArrayAppendPtr(_vertices, &_a);
    _tpVec2ArrayAppendPtr(_vertices, &_b);
    _tpVec2ArrayAppendPtr(_vertices, &_c);
}

TARP_LOCAL void _tpGLPushQuad(_tpVec2Array * _vertices, tpVec2 _a, tpVec2 _b, tpVec2 _c, tpVec2 _d)
{
    _tpVec2ArrayAppendPtr(_vertices, &_a);
    _tpVec2ArrayAppendPtr(_vertices, &_b);
    _tpVec2ArrayAppendPtr(_vertices, &_c);
    _tpVec2ArrayAppendPtr(_vertices, &_c);
    _tpVec2ArrayAppendPtr(_vertices, &_d);
    _tpVec2ArrayAppendPtr(_vertices, &_a);
}

TARP_LOCAL void _tpGLMakeCircleSector(
    tpVec2 _center, tpVec2 _r0, tpVec2 _r1, tpFloat _levelOfDetail, _tpVec2Array * _outVertices)
{
    tpMat2 rot;
    tpVec2 r, current, last;
    tpFloat stepSize;

    /* @TODO: The step size should depend on _levelOfDetail. */
    stepSize = TARP_PI / 16;
    rot = tpMat2MakeRotation(stepSize);
    r = _r0;
    last = tpVec2Add(_center, r);
    for (;;)
    {
        r = tpMat2MultVec2(&rot, r);
        if (tpVec2Cross(r, _r1) < 0)
        {
            break;
        }
        current = tpVec2Add(_center, r);
        _tpGLPushTriangle(_outVertices, _center, last, current);
        last = current;
    }
    current = tpVec2Add(_center, _r1);
    _tpGLPushTriangle(_outVertices, _center, last, current);
}

TARP_LOCAL void _tpGLMakeJoinBevel(tpVec2 _lePrev,
                                   tpVec2 _rePrev,
                                   tpVec2 _le,
                                   tpVec2 _re,
                                   tpFloat _cross,
                                   _tpVec2Array * _outVertices)
{
    _tpGLPushTriangle(_outVertices, _lePrev, (_cross < 0) ? _le : _re, _rePrev);
}

TARP_LOCAL tpBool
_tpGLIntersectLines(tpVec2 _p0, tpVec2 _d0, tpVec2 _p1, tpVec2 _d1, tpVec2 * _outResult)
{
    tpFloat cross, t;
    tpVec2 delta, doff;

    delta = tpVec2Sub(_p0, _p1);
    cross = tpVec2Cross(_d0, _d1);

    /* lines are parallel */
    if (cross == 0.0)
        return tpFalse;

    t = tpVec2Cross(_d1, delta) / cross;
    doff = tpVec2MultScalar(_d0, t);
    *_outResult = tpVec2Add(_p0, doff);

    return tpTrue;
}

TARP_LOCAL void _tpGLMakeJoinMiter(tpVec2 _p,
                                   tpVec2 _e0,
                                   tpVec2 _e1,
                                   tpVec2 _dir0,
                                   tpVec2 _dir1,
                                   tpFloat _cross,
                                   _tpVec2Array * _outVertices)
{
    tpVec2 intersection;

    /* tpVec2 inv = tpVec2MultScalar(_dir1, -1); */
    _tpGLIntersectLines(_e0, _dir0, _e1, _dir1, &intersection);

    _tpGLPushQuad(_outVertices, _p, _e0, intersection, _e1);
}

TARP_LOCAL void _tpGLMakeJoin(tpStrokeJoin _type,
                              tpVec2 _p,
                              tpVec2 _dir0,
                              tpVec2 _dir1,
                              tpVec2 _perp0,
                              tpVec2 _perp1,
                              tpVec2 _lePrev,
                              tpVec2 _rePrev,
                              tpVec2 _le,
                              tpVec2 _re,
                              tpFloat _cross,
                              tpFloat _miterLimit,
                              _tpVec2Array * _outVertices)
{
    tpVec2 nperp0, nperp1;
    tpFloat miterLen, theta, levelOfDetail;

    switch (_type)
    {
    case kTpStrokeJoinRound:
        levelOfDetail = 0; /* TODO: levelOfDetail should depend on the
                              stroke width and the transform */
        if (_cross < 0.0f)
        {
            _tpGLMakeCircleSector(_p, _perp0, _perp1, levelOfDetail, _outVertices);
        }
        else
        {
            /* negate the perpendicular vectors and reorder function
             * arguments */
            tpVec2 flippedPerp0, flippedPerp1;
            flippedPerp0 = tpVec2Make(-_perp0.x, -_perp0.y);
            flippedPerp1 = tpVec2Make(-_perp1.x, -_perp1.y);
            _tpGLMakeCircleSector(_p, flippedPerp1, flippedPerp0, levelOfDetail, _outVertices);
        }
        break;
    case kTpStrokeJoinMiter:
        nperp0 = tpVec2Perp(_dir0);
        nperp1 = tpVec2Perp(_dir1);
        theta = acos(tpVec2Dot(nperp0, nperp1));

        /* make sure we have the shortest angle */
        if (theta > TARP_HALF_PI)
            theta = TARP_PI - theta;

        miterLen = 1.0 / sin(theta / 2.0);
        if (miterLen < _miterLimit)
        {
            if (_cross < 0.0f)
            {
                _tpGLMakeJoinMiter(_p, _lePrev, _le, _dir0, _dir1, _cross, _outVertices);
            }
            else
            {
                _tpGLMakeJoinMiter(_p, _rePrev, _re, _dir0, _dir1, _cross, _outVertices);
            }
            break;
        }
    /* fall back to bevel */
    case kTpStrokeJoinBevel:
    default:
        _tpGLMakeJoinBevel(_lePrev, _rePrev, _le, _re, _cross, _outVertices);
        break;
    }
}

TARP_LOCAL void _tpGLMakeCapSquare(
    tpVec2 _p, tpVec2 _dir, tpVec2 _le, tpVec2 _re, _tpVec2Array * _outVertices)
{
    tpVec2 a, b;
    a = tpVec2Add(_re, _dir);
    b = tpVec2Add(_le, _dir);

    _tpGLPushQuad(_outVertices, _re, a, b, _le);
}

TARP_LOCAL void _tpGLMakeCap(tpStrokeCap _type,
                             tpVec2 _p,
                             tpVec2 _dir,
                             tpVec2 _perp,
                             tpVec2 _le,
                             tpVec2 _re,
                             tpBool _bStart,
                             _tpVec2Array * _outVertices)
{
    tpFloat levelOfDetail;
    tpVec2 flippedPerp;
    switch (_type)
    {
    case kTpStrokeCapRound:
        /* TODO: levelOfDetail should depend on the stroke width and the
         * transform */
        levelOfDetail = 0;
        flippedPerp = tpVec2Make(-_perp.x, -_perp.y);
        _tpGLMakeCircleSector(_p, _perp, flippedPerp, levelOfDetail, _outVertices);
        break;
    case kTpStrokeCapSquare:
        _tpGLMakeCapSquare(_p, _dir, _le, _re, _outVertices);
        break;
    case kTpStrokeCapButt:
    default:
        break;
    }
}

TARP_LOCAL void _tpGLRenderCacheContourContinousStrokeGeometry(_tpGLRenderCacheContour * _contour,
                                                               const tpStyle * _style,
                                                               _tpVec2Array * _outVertices,
                                                               _tpBoolArray * _outJoints)
{
    int j, voff;
    tpVec2 p0, p1, dir, perp, dirPrev, perpPrev;
    tpVec2 le0, le1, re0, re1, lePrev, rePrev;
    tpVec2 firstDir, firstPerp, firstLe, firstRe;
    tpFloat cross, halfSw;

    voff = _outVertices->count;
    halfSw = _style->strokeWidth * 0.5;
    _contour->strokeVertexOffset = voff;

    /* @TODO: This needed? */
    if (_contour->fillVertexCount <= 1)
    {
        _contour->strokeVertexCount = 0;
        return;
    }

    for (j = _contour->fillVertexOffset;
         j < _contour->fillVertexOffset + _contour->fillVertexCount - 1;
         ++j)
    {
        p0 = _tpVec2ArrayAt(_outVertices, j);
        p1 = _tpVec2ArrayAt(_outVertices, j + 1);
        dir = tpVec2Sub(p1, p0);
        tpVec2NormalizeSelf(&dir);
        perp.x = dir.y * halfSw;
        perp.y = -dir.x * halfSw;

        le0 = tpVec2Add(p0, perp);
        re0 = tpVec2Sub(p0, perp);
        le1 = tpVec2Add(p1, perp);
        re1 = tpVec2Sub(p1, perp);

        /* check if this is the first segment / dash start */
        if (j == _contour->fillVertexOffset)
        {
            if (!_contour->bIsClosed)
            {
                /* start cap? */
                firstDir = tpVec2MultScalar(dir, -1 * halfSw);
                firstPerp.x = firstDir.y;
                firstPerp.y = -firstDir.x;
                _tpGLMakeCap(
                    _style->strokeCap, p0, firstDir, firstPerp, le0, re0, tpTrue, _outVertices);
            }
            else if (j == _contour->fillVertexOffset)
            {
                /*
                if the contour is closed, we cache the directions and
                edges to be used for the last segments stroke
                calculations
                */
                firstDir = dir;
                firstPerp = perp;
                firstLe = le0;
                firstRe = re0;
            }
        }
        else
        {
            cross = tpVec2Cross(perp, perpPrev);
            /* check if this is a joint */
            if (_tpBoolArrayAt(_outJoints, j))
            {
                _tpGLMakeJoin(_style->strokeJoin,
                              p0,
                              dirPrev,
                              dir,
                              perpPrev,
                              perp,
                              lePrev,
                              rePrev,
                              le0,
                              re0,
                              cross,
                              _style->miterLimit,
                              _outVertices);
            }
            else
            {
                /* by default we join consecutive segment quads with a
                 * bevel */
                _tpGLMakeJoinBevel(lePrev, rePrev, le0, re0, cross, _outVertices);
            }
        }

        /* add the quad for the current segment */
        _tpGLPushQuad(_outVertices, le1, le0, re0, re1);

        /* check if we need to do the end cap / join */
        if (j == _contour->fillVertexOffset + _contour->fillVertexCount - 2 ||
            _contour->fillVertexCount == 2)
        {
            if (_tpBoolArrayAt(_outJoints, j + 1) && _contour->bIsClosed)
            {
                /* last join */
                cross = tpVec2Cross(firstPerp, perp);
                _tpGLMakeJoin(_style->strokeJoin,
                              p1,
                              dir,
                              firstDir,
                              perp,
                              firstPerp,
                              le1,
                              re1,
                              firstLe,
                              firstRe,
                              cross,
                              _style->miterLimit,
                              _outVertices);
            }
            else
            {
                /* end cap */
                firstDir = tpVec2MultScalar(dir, halfSw);
                _tpGLMakeCap(
                    _style->strokeCap, p1, firstDir, perp, le1, re1, tpFalse, _outVertices);
            }
        }

        perpPrev = perp;
        dirPrev = dir;
        lePrev = le1;
        rePrev = re1;
    }

    _contour->strokeVertexCount = _outVertices->count - voff;
}

/* helper struct that encapsulates helper info about the start of the dash pattern */
typedef struct STICK_LOCAL
{
    int startDashIndex;   /* the start index of the dash pattern (calculated from the dashOffset of
                             the style) */
    tpBool bStartDashOn;  /* is the start of the contour on a dash or negative space? */
    tpFloat startDashLen; /* how much length is left of the initial dash? */
} _tpGLDashStartState;

TARP_LOCAL void _tpGLRenderCacheContourDashedStrokeGeometry(
    _tpGLRenderCacheContour * _contour,
    const tpStyle * _style,
    const _tpGLDashStartState * _startDashState,
    _tpVec2Array * _vertices,
    _tpBoolArray * _joints)
{
    int j, dashIndex, voff;
    tpVec2 p0, p1, dir, perp, dirPrev, perpPrev;
    tpVec2 le0, le1, re0, re1, lePrev, rePrev;
    tpVec2 firstDir, firstPerp, firstLe, firstRe;
    tpVec2 dirr;
    tpFloat cross, halfSw, dashOffset, dashLen, segmentOff, segmentLen;
    tpBool bDashStart, bFirstDashMightNeedJoin, bLastSegment;
    tpBool bOnDash, bBarelyJoined;

    voff = _vertices->count;
    _contour->strokeVertexOffset = voff;
    dashIndex = _startDashState->startDashIndex;
    dashOffset = 0;
    dashLen = _startDashState->startDashLen;
    bOnDash = _startDashState->bStartDashOn;
    bDashStart = tpTrue;
    bFirstDashMightNeedJoin = tpFalse;
    bBarelyJoined = tpFalse;
    halfSw = _style->strokeWidth * 0.5f;

    for (j = _contour->fillVertexOffset;
         j < _contour->fillVertexOffset + _contour->fillVertexCount - 1;
         ++j)
    {
        bLastSegment = (tpBool)(j == _contour->fillVertexOffset + _contour->fillVertexCount - 2);

        p0 = _tpVec2ArrayAt(_vertices, j);
        p1 = _tpVec2ArrayAt(_vertices, j + 1);
        dir = tpVec2Sub(p1, p0);
        segmentLen = tpVec2Length(dir);
        segmentOff = 0;
        dir.x /= segmentLen;
        dir.y /= segmentLen;

        perp.x = dir.y * halfSw;
        perp.y = -dir.x * halfSw;
        cross = tpVec2Cross(perp, perpPrev);

        /* check if this is a joint */
        if (bOnDash && _tpBoolArrayAt(_joints, j))
        {
            le0 = tpVec2Add(p0, perp);
            re0 = tpVec2Sub(p0, perp);
            _tpGLMakeJoin(_style->strokeJoin,
                          p0,
                          dirPrev,
                          dir,
                          perpPrev,
                          perp,
                          lePrev,
                          rePrev,
                          le0,
                          re0,
                          cross,
                          _style->miterLimit,
                          _vertices);
        }

        do
        {
            tpFloat left = TARP_MIN((segmentLen - segmentOff), dashLen - dashOffset);
            dirr = tpVec2MultScalar(dir, left);
            p1 = tpVec2Add(p0, dirr);

            le0 = tpVec2Add(p0, perp);
            re0 = tpVec2Sub(p0, perp);
            le1 = tpVec2Add(p1, perp);
            re1 = tpVec2Sub(p1, perp);

            if (bDashStart && bOnDash)
            {
                bDashStart = tpFalse;

                /*
                if the current contour is not closed, or we are not at
                the beginning of the first segment of the contour, add a
                starting cap to the dash...
                */
                if (!_contour->bIsClosed || j != _contour->fillVertexOffset || segmentOff > 0)
                {
                    tpVec2 tmpDir, tmpPerp;
                    tmpDir = tpVec2MultScalar(dir, -1 * halfSw);
                    tmpPerp.x = tmpDir.y;
                    tmpPerp.y = -tmpDir.x;
                    _tpGLMakeCap(
                        _style->strokeCap, p0, tmpDir, tmpPerp, le0, re0, tpTrue, _vertices);
                }
                /*
                ...otherwise cache the initial values for the cap
                computation and mark that the contour might need a
                closing join in case the first and last dash of the
                contour touch.
                */
                else if (j == _contour->fillVertexOffset)
                {
                    bFirstDashMightNeedJoin = tpTrue;
                    firstDir = dir;
                    firstPerp = perp;
                    firstLe = le0;
                    firstRe = re0;
                }
            }
            else if (!bDashStart && bOnDash)
            {
                _tpGLMakeJoinBevel(lePrev, rePrev, le0, re0, cross, _vertices);
            }

            if (bOnDash)
            {
                /* add the quad for the current dash on the current
                 * segment */
                _tpGLPushQuad(_vertices, le1, le0, re0, re1);
            }

            dashOffset += left;
            segmentOff += left;
            p0 = p1;

            if (dashOffset >= dashLen)
            {
                if (bOnDash)
                {
                    /* dont make cap if the first and last dash of the
                     * contour touch and the last dash is not finished.
                     */
                    if (!bFirstDashMightNeedJoin || !bLastSegment || segmentLen - segmentOff > 0)
                    {
                        _tpGLMakeCap(
                            _style->strokeCap, p1, dir, perp, le1, re1, tpFalse, _vertices);
                    }
                    else
                    {
                        bBarelyJoined = tpTrue;
                    }
                }

                dashOffset = 0;
                dashIndex = (dashIndex + 1) % _style->dashCount;
                dashLen = _style->dashArray[dashIndex];
                bDashStart = tpTrue;
                bOnDash = (tpBool)!bOnDash;
            }

        } while ((segmentLen - segmentOff) > 0);

        if (bLastSegment)
        {
            /* if the first and last dash of the contour touch, we
             * connect them with a join */
            if (bFirstDashMightNeedJoin)
            {
                if ((bBarelyJoined || (dashOffset > 0 && bOnDash)))
                {
                    cross = tpVec2Cross(firstPerp, perp);
                    _tpGLMakeJoin(_style->strokeJoin,
                                  p1,
                                  dir,
                                  firstDir,
                                  perp,
                                  firstPerp,
                                  le1,
                                  re1,
                                  firstLe,
                                  firstRe,
                                  cross,
                                  _style->miterLimit,
                                  _vertices);
                }
                else
                {
                    /* otherwise we simply add a starting cap to the
                     * first dash of the contour... */
                    tpVec2 tmpDir, tmpPerp;
                    tmpDir = tpVec2MultScalar(firstDir, -1 * halfSw);
                    tmpPerp.x = tmpDir.y;
                    tmpPerp.y = -tmpDir.x;
                    _tpGLMakeCap(_style->strokeCap,
                                 p1,
                                 tmpDir,
                                 tmpPerp,
                                 firstRe,
                                 firstLe,
                                 tpFalse,
                                 _vertices);
                }
            }
            else if (dashOffset > 0 && bOnDash)
            {
                _tpGLMakeCap(_style->strokeCap, p1, dir, perp, le1, re1, tpFalse, _vertices);
            }
        }

        dashLen -= dashOffset;
        dashOffset = 0.0;

        perpPrev = perp;
        dirPrev = dir;
        lePrev = le1;
        rePrev = re1;
    }

    _contour->strokeVertexCount = _vertices->count - voff;
}

TARP_LOCAL void _tpGLStroke(_tpGLPath * _path,
                            _tpGLRenderCacheContourArray * _contours,
                            tpBool _bIsRebuildingInternalCache,
                            _tpGLRenderCache * _oldCache,
                            const tpStyle * _style,
                            _tpVec2Array * _vertices,
                            _tpBoolArray * _joints,
                            int * _outStrokeVertexCount)
{
    int i;
    _tpGLContour * c;
    _tpGLRenderCacheContour * rc;
    _tpGLDashStartState dashStartState;

    assert(_vertices->count == _joints->count);

    /* compute the start of the dash pattern if needed */
    if (_style->dashCount)
    {
        tpFloat patternLen, offsetIntoPattern;

        /* no dash offset */
        if (_style->dashOffset == 0)
        {
            dashStartState.startDashIndex = 0;
            dashStartState.bStartDashOn = tpTrue;
            dashStartState.startDashLen = _style->dashArray[dashStartState.startDashIndex];
        }
        else
        {
            /* compute offset into dash pattern */
            patternLen = 0;
            for (i = 0; i < _style->dashCount; ++i)
            {
                patternLen += _style->dashArray[i];
            }

            offsetIntoPattern = _style->dashOffset;
            if (fabs(offsetIntoPattern) >= patternLen)
            {
                offsetIntoPattern = fmod(offsetIntoPattern, patternLen);
            }

            dashStartState.startDashLen = -offsetIntoPattern;
            if (_style->dashOffset > 0)
            {
                dashStartState.startDashIndex = 0;
                while (dashStartState.startDashLen <= 0.0)
                {
                    dashStartState.startDashLen += _style->dashArray[dashStartState.startDashIndex];
                    dashStartState.startDashIndex++;
                }
                dashStartState.startDashIndex = TARP_MAX(0, dashStartState.startDashIndex - 1);
                dashStartState.bStartDashOn = (tpBool) !(dashStartState.startDashIndex % 2);
            }
            else
            {
                dashStartState.startDashIndex = _style->dashCount;
                while (dashStartState.startDashLen > 0.0f)
                {
                    dashStartState.startDashIndex--;
                    dashStartState.startDashLen -= _style->dashArray[dashStartState.startDashIndex];
                }
                dashStartState.startDashIndex = TARP_MAX(0, dashStartState.startDashIndex);
                dashStartState.bStartDashOn = (tpBool) !(dashStartState.startDashIndex % 2);
                dashStartState.startDashLen =
                    _style->dashArray[dashStartState.startDashIndex] + dashStartState.startDashLen;
            }
        }
    }

    *_outStrokeVertexCount = 0;
    for (i = 0; i < _contours->count; ++i)
    {
        rc = _tpGLRenderCacheContourArrayAtPtr(_contours, i);
        c = _tpGLContourArrayAtPtr(&_path->contours, i);

        if (c->bDirty || !_oldCache)
        {
            /* only undo the dirty flag if the internal path cache is currently being rebuild! */
            if (_bIsRebuildingInternalCache)
                c->bDirty = tpFalse;

            if (_style->dashCount)
            {
                _tpGLRenderCacheContourDashedStrokeGeometry(
                    rc, _style, &dashStartState, _vertices, _joints);
            }
            else
            {
                _tpGLRenderCacheContourContinousStrokeGeometry(rc, _style, _vertices, _joints);
            }
        }
        else
        {
            /* grab the contour from the old cache and copy the old stroke data for the contour */
            rc = _tpGLRenderCacheContourArrayAtPtr(&_oldCache->contours, i);
            _tpVec2ArrayAppendCArray(
                _vertices,
                _tpVec2ArrayAtPtr(&_oldCache->geometryCache, rc->strokeVertexOffset),
                rc->strokeVertexCount);
        }

        *_outStrokeVertexCount += rc->strokeVertexCount;
    }
}

TARP_LOCAL void _tpGLFlattenCurve(const _tpGLCurve * _curve,
                                  tpFloat _angleTolerance,
                                  int _bIsClosed,
                                  int _bFirstCurve,
                                  int _bLastCurve,
                                  _tpVec2Array * _outVertices,
                                  _tpBoolArray * _outJoints,
                                  _tpGLRect * _bounds,
                                  int * _vertexCount)
{
    _tpGLCurve stack[TARP_MAX_CURVE_SUBDIVISIONS];
    _tpGLCurvePair cp;
    _tpGLCurve * current;
    int stackIndex = 0;
    stack[0] = *_curve;

    while (stackIndex >= 0)
    {
        current = &stack[stackIndex];

        if (stackIndex < TARP_MAX_CURVE_SUBDIVISIONS - 1 &&
            !_tpGLIsCurveFlatEnough(current, _angleTolerance))
        {
            /*
            subdivide curve and add continue with the left of the two curves
            by putting it on top of the stack.
            */
            _tpGLSubdivideCurve(current, 0.5, &cp);
            *current = cp.second;
            stack[++stackIndex] = cp.first;
        }
        else
        {
            /* for the first curve we also add its first segment */
            if (_bFirstCurve)
            {
                _tpVec2ArrayAppendPtr(_outVertices, &current->p0);
                _tpVec2ArrayAppendPtr(_outVertices, &current->p1);

                _tpBoolArrayAppend(_outJoints, tpFalse);
                _tpBoolArrayAppend(_outJoints,
                                   (tpBool)(tpVec2Equals(current->p1, _curve->p1) && !_bLastCurve));
                _tpGLEvaluatePointForBounds(current->p0, _bounds);
                _tpGLEvaluatePointForBounds(current->p1, _bounds);
                *_vertexCount += 2;

                /* we don't want to do this for the following subdivisions
                 */
                _bFirstCurve = 0;
            }
            else
            {
                _tpVec2ArrayAppendPtr(_outVertices, &current->p1);

                _tpBoolArrayAppend(
                    _outJoints,
                    (tpBool)(_bIsClosed ? tpVec2Equals(current->p1, _curve->p1)
                                        : (tpVec2Equals(current->p1, _curve->p1) && !_bLastCurve)));
                _tpGLEvaluatePointForBounds(current->p1, _bounds);
                (*_vertexCount)++;
            }
            stackIndex--;
        }
    }
}

TARP_LOCAL void _tpGLInitBounds(_tpGLRect * _bounds)
{
    _bounds->min.x = FLT_MAX;
    _bounds->min.y = FLT_MAX;
    _bounds->max.x = -FLT_MAX;
    _bounds->max.y = -FLT_MAX;
}

TARP_LOCAL void _tpGLMergeBounds(_tpGLRect * _a, const _tpGLRect * _b)
{
    _tpGLEvaluatePointForBounds(_b->min, _a);
    _tpGLEvaluatePointForBounds(_b->max, _a);
}

TARP_LOCAL tpBool _tpGLFlattenContour(_tpGLContour * _contour,
                                      tpFloat _angleTolerance,
                                      const tpTransform * _transform,
                                      _tpVec2Array * _outVertices,
                                      _tpBoolArray * _outJoints,
                                      _tpGLRect * _mergeBounds,
                                      _tpGLRenderCacheContour * _outContour)
{
    int off, vcount, j;
    _tpGLRect contourBounds;
    _tpGLCurve curve;
    _tpGLRenderCacheContour renderContour;
    tpVec2 lastTransformedPos;
    tpSegment *last = NULL, *current = NULL;

    vcount = 0;
    off = _outVertices->count;
    _tpGLInitBounds(&contourBounds);

    last = _tpSegmentArrayAtPtr(&_contour->segments, 0);
    for (j = 1; j < _contour->segments.count; ++j)
    {
        current = _tpSegmentArrayAtPtr(&_contour->segments, j);

        curve.p0 = tpVec2Make(last->position.x, last->position.y);
        curve.h0 = tpVec2Make(last->handleOut.x, last->handleOut.y);
        curve.h1 = tpVec2Make(current->handleIn.x, current->handleIn.y);
        curve.p1 = tpVec2Make(current->position.x, current->position.y);

        if (_transform)
        {
            curve.p0 = j > 1 ? lastTransformedPos : tpTransformApply(_transform, curve.p0);
            curve.h0 = tpTransformApply(_transform, curve.h0);
            curve.h1 = tpTransformApply(_transform, curve.h1);
            curve.p1 = tpTransformApply(_transform, curve.p1);
            lastTransformedPos = curve.p1;
        }

        _tpGLFlattenCurve(&curve,
                          _angleTolerance,
                          _contour->bIsClosed,
                          j == 1,
                          tpFalse,
                          _outVertices,
                          _outJoints,
                          &contourBounds,
                          &vcount);

        last = current;
    }

    /* if the contour is closed, flatten the last closing curve */
    if (_contour->bIsClosed && _contour->segments.count &&
        tpVec2Distance(
            _tpSegmentArrayAtPtr(&_contour->segments, 0)->position,
            _tpSegmentArrayAtPtr(&_contour->segments, _contour->segments.count - 1)->position) >
            FLT_EPSILON)
    {
        tpSegment * fs = _tpSegmentArrayAtPtr(&_contour->segments, 0);

        curve.p0 = tpVec2Make(last->position.x, last->position.y);
        curve.h0 = tpVec2Make(last->handleOut.x, last->handleOut.y);
        curve.h1 = tpVec2Make(fs->handleIn.x, fs->handleIn.y);
        curve.p1 = tpVec2Make(fs->position.x, fs->position.y);

        if (_transform)
        {
            /* @TODO: are there cases were lastTransformedPos can
             * still be uninitialized?!?! */
            curve.p0 = lastTransformedPos;
            curve.h0 = tpTransformApply(_transform, curve.h0);
            curve.h1 = tpTransformApply(_transform, curve.h1);
            curve.p1 = tpTransformApply(_transform, curve.p1);
        }

        _tpGLFlattenCurve(&curve,
                          _angleTolerance,
                          _contour->bIsClosed,
                          tpFalse,
                          tpTrue,
                          _outVertices,
                          _outJoints,
                          &contourBounds,
                          &vcount);
    }

    renderContour.fillVertexOffset = off;
    renderContour.fillVertexCount = vcount;
    renderContour.bIsClosed = _contour->bIsClosed;
    _contour->bounds = contourBounds;

    *_outContour = renderContour;
    _tpGLMergeBounds(_mergeBounds, &contourBounds);
    return tpFalse;
}

TARP_LOCAL int _tpGLColorStopComp(const void * _a, const void * _b)
{
    if (((tpColorStop *)_a)->offset < ((tpColorStop *)_b)->offset)
        return -1;
    return 1;
}

TARP_LOCAL void _tpGLFinalizeColorStops(_tpGLContext * _ctx, _tpGLGradient * _grad)
{
    int i, j;
    tpColorStop * current;
    tpBool bAdd, bHasStartStop, bHasEndStop;

    if (!_grad->stops.count)
        return;

    _tpColorStopArrayClear(&_ctx->tmpColorStops);

    bHasStartStop = tpFalse;
    bHasEndStop = tpFalse;

    /* remove duplicates */
    for (i = 0; i < _grad->stops.count; ++i)
    {
        bAdd = tpTrue;
        current = _tpColorStopArrayAtPtr(&_grad->stops, i);
        if (current->offset == 0)
            bHasStartStop = tpTrue;
        else if (current->offset == 1)
            bHasEndStop = tpTrue;

        for (j = 0; j < _ctx->tmpColorStops.count; ++j)
        {
            if (current->offset == _tpColorStopArrayAtPtr(&_ctx->tmpColorStops, j)->offset)
            {
                bAdd = tpFalse;
                break;
            }
        }

        if (bAdd && current->offset >= 0 && current->offset <= 1)
            _tpColorStopArrayAppendPtr(&_ctx->tmpColorStops, current);
    }

    /* sort from 0 - 1 by offset */
    qsort(_ctx->tmpColorStops.array,
          _ctx->tmpColorStops.count,
          sizeof(tpColorStop),
          _tpGLColorStopComp);

    /* make sure there is a stop at 0 and 1 offset */
    if (!bHasStartStop || !bHasEndStop)
    {
        tpColorStop tmp;

        _tpColorStopArrayClear(&_grad->stops);
        if (!bHasStartStop)
        {
            tmp.color = _tpColorStopArrayAtPtr(&_ctx->tmpColorStops, 0)->color;
            tmp.offset = 0;
            _tpColorStopArrayAppendPtr(&_grad->stops, &tmp);
        }

        _tpColorStopArrayAppendArray(&_grad->stops, &_ctx->tmpColorStops);

        if (!bHasEndStop)
        {
            tmp.color = _tpColorStopArrayLastPtr(&_ctx->tmpColorStops)->color;
            tmp.offset = 1;
            _tpColorStopArrayAppendPtr(&_grad->stops, &tmp);
        }
    }
    else
    {
        /* if they are already there, we can simply swap */
        _tpColorStopArraySwap(&_grad->stops, &_ctx->tmpColorStops);
    }
}

TARP_LOCAL void _tpGLUpdateRampTexture(_tpGLGradient * _grad)
{
    tpColor pixels[TARP_GL_RAMP_TEXTURE_SIZE] = { 0 };
    int xStart, xEnd, diff, i, j;
    tpFloat mixFact;
    tpColor mixColor;
    tpColorStop *stop1, *stop2;

    /* generate the ramp texture */
    xStart = 0;
    xEnd = 0;

    if (_grad->stops.count)
    {
        stop1 = _tpColorStopArrayAtPtr(&_grad->stops, 0);
        pixels[0] = stop1->color;

        for (i = 1; i < _grad->stops.count; ++i)
        {
            stop2 = _tpColorStopArrayAtPtr(&_grad->stops, i);
            xEnd = (int)(stop2->offset * (TARP_GL_RAMP_TEXTURE_SIZE - 1));

            assert(xStart >= 0 && xStart < TARP_GL_RAMP_TEXTURE_SIZE && xEnd >= 0 &&
                   xEnd < TARP_GL_RAMP_TEXTURE_SIZE && xStart <= xEnd);

            diff = xEnd - xStart;
            mixColor.r = stop2->color.r - stop1->color.r;
            mixColor.g = stop2->color.g - stop1->color.g;
            mixColor.b = stop2->color.b - stop1->color.b;
            mixColor.a = stop2->color.a - stop1->color.a;

            for (j = xStart + 1; j <= xEnd; ++j)
            {
                mixFact = (tpFloat)(j - xStart) / (tpFloat)(diff);
                pixels[j].r = stop1->color.r + mixColor.r * mixFact;
                pixels[j].g = stop1->color.g + mixColor.g * mixFact;
                pixels[j].b = stop1->color.b + mixColor.b * mixFact;
                pixels[j].a = stop1->color.a + mixColor.a * mixFact;
            }
            stop1 = stop2;
            xStart = xEnd;
        }
    }

    _TARP_ASSERT_NO_GL_ERROR(glActiveTexture(GL_TEXTURE0));
    _TARP_ASSERT_NO_GL_ERROR(glBindTexture(GL_TEXTURE_1D, _grad->rampTexture));
    _TARP_ASSERT_NO_GL_ERROR(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    _TARP_ASSERT_NO_GL_ERROR(glTexSubImage1D(
        GL_TEXTURE_1D, 0, 0, TARP_GL_RAMP_TEXTURE_SIZE, GL_RGBA, GL_FLOAT, &pixels[0].r));
}

TARP_LOCAL void _tpGLUpdateVAO(_tpGLVAO * _vao, void * _data, int _byteCount)
{
    /* @TODO: not sure if this buffer orphaning style data upload makes a
     * difference these days anymore. (TEST??) */
    if ((GLuint)_byteCount > _vao->vboSize)
    {
        _TARP_ASSERT_NO_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, _byteCount, _data, GL_DYNAMIC_DRAW));
        _vao->vboSize = _byteCount;
    }
    else
    {
        _TARP_ASSERT_NO_GL_ERROR(
            glBufferData(GL_ARRAY_BUFFER, _vao->vboSize, NULL, GL_DYNAMIC_DRAW));
        _TARP_ASSERT_NO_GL_ERROR(glBufferSubData(GL_ARRAY_BUFFER, 0, _byteCount, _data));
    }
}

TARP_LOCAL void _tpGLDrawPaint(_tpGLContext * _ctx,
                               const _tpGLRenderCache * _cache,
                               const tpPaint * _paint,
                               const _tpGLGradientCacheData * _gradCache)
{
    if (_paint->type == kTpPaintTypeColor)
    {
        _TARP_ASSERT_NO_GL_ERROR(glUniform4fv(_ctx->meshColorLoc, 1, &_paint->data.color.r));
        _TARP_ASSERT_NO_GL_ERROR(glDrawArrays(GL_TRIANGLE_STRIP, _cache->boundsVertexOffset, 4));
    }
    else if (_paint->type == kTpPaintTypeGradient)
    {
        _tpGLGradient * grad = (_tpGLGradient *)_paint->data.gradient.pointer;

        /* bind the gradient's texture */
        _TARP_ASSERT_NO_GL_ERROR(glActiveTexture(GL_TEXTURE0));
        _TARP_ASSERT_NO_GL_ERROR(glBindTexture(GL_TEXTURE_1D, grad->rampTexture));

        _TARP_ASSERT_NO_GL_ERROR(glUseProgram(_ctx->textureProgram));
        _TARP_ASSERT_NO_GL_ERROR(
            glUniformMatrix4fv(_ctx->tpTextureLoc, 1, GL_FALSE, &_cache->renderMatrix.v[0]));
        _TARP_ASSERT_NO_GL_ERROR(glBindVertexArray(_ctx->textureVao.vao));
        _TARP_ASSERT_NO_GL_ERROR(
            glDrawArrays(GL_TRIANGLE_FAN, _gradCache->vertexOffset, _gradCache->vertexCount));
        _TARP_ASSERT_NO_GL_ERROR(glUseProgram(_ctx->program));
        _TARP_ASSERT_NO_GL_ERROR(glBindVertexArray(_ctx->vao.vao));
    }
}

TARP_LOCAL void _tpGLCacheBoundsGeometry(_tpGLRenderCache * _cache, const tpStyle * _style)
{
    _tpGLRect bounds;
    _tpGLRect * bptr;
    tpVec2 boundsData[4];

    _cache->boundsVertexOffset = _cache->geometryCache.count;

    /*
    add the bounds geometry to the end of the geometry cache.
    If there is a stroke, upload the stroke bounds instead
    */
    if (_style->stroke.type != kTpPaintTypeNone)
    {
        tpFloat adder;
        bounds = _cache->boundsCache;
        /*
        for miter we don't calculate a tight bounding box but instead
        increase it to cover the worst case based on the stroke width and
        miter limit.
        @TODO: Make sure that this is actually good enough of a solution
        */
        adder = _style->strokeJoin == kTpStrokeJoinMiter
                    ? _style->miterLimit * _style->strokeWidth * 0.5f
                    : _style->strokeWidth;
        bounds.min.x -= adder;
        bounds.min.y -= adder;
        bounds.max.x += adder;
        bounds.max.y += adder;

        _cache->strokeBoundsCache = bounds;
        bptr = &bounds;
    }
    else
    {
        bptr = &_cache->boundsCache;
    }

    boundsData[0] = bptr->min;
    boundsData[1] = tpVec2Make(bptr->min.x, bptr->max.y);
    boundsData[2] = tpVec2Make(bptr->max.x, bptr->min.y);
    boundsData[3] = bptr->max;

    _tpVec2ArrayAppendCArray(&_cache->geometryCache, boundsData, 4);
}

typedef struct TARP_LOCAL
{
    tpVec2 vertex;
    tpFloat tc;
} TexVertex;

TARP_LOCAL void _tpGLGradientLinearGeometry(_tpGLContext * _ctx,
                                            _tpGLGradient * _grad,
                                            const tpTransform * _paintTransform,
                                            const _tpGLRect * _bounds,
                                            tpBool _bIsScalingStroke,
                                            _tpGLTextureVertexArray * _vertices,
                                            int * _outVertexOffset,
                                            int * _outVertexCount)
{
    /* regenerate the geometry for this path/gradient combo */
    _tpGLTextureVertex vertices[4];
    tpVec2 dir, ndir, dest, origin;
    tpVec2 tmp;
    tpFloat len2;
    int i;

    origin = tpTransformApply(_paintTransform, _grad->origin);
    dest = tpTransformApply(_paintTransform, _grad->destination);

    /* if we are not scaling the stroke, the geometry will be transformed
    before being send to the GPU, hence we need to transform the gradient
    geometry, too! */
    if (!_bIsScalingStroke)
    {
        origin = tpTransformApply(&_ctx->transform, origin);
        dest = tpTransformApply(&_ctx->transform, dest);
    }

    dir = tpVec2Sub(dest, origin);
    len2 = tpVec2LengthSquared(dir);
    ndir = tpVec2MultScalar(dir, 1 / len2);

    vertices[0].vertex = _bounds->min;
    vertices[1].vertex = tpVec2Make(_bounds->max.x, _bounds->min.y);
    vertices[2].vertex = _bounds->max;
    vertices[3].vertex = tpVec2Make(_bounds->min.x, _bounds->max.y);

    for (i = 0; i < 4; ++i)
    {
        tmp = tpVec2Sub(vertices[i].vertex, origin);
        vertices[i].tc.x = tpVec2Dot(tmp, ndir);
        vertices[i].tc.y = 0;
    }

    *_outVertexOffset = _vertices->count;
    *_outVertexCount = 4;
    _tpGLTextureVertexArrayAppendCArray(_vertices, vertices, 4);
}

TARP_LOCAL tpFloat _tpLineUnitCircleIntersection(tpVec2 _p0, tpVec2 _p1)
{
    /* Find t such that tpVec2Lerp(p0, p1, t) lies on the unit circle
     * (taking the larger of two solutions) */
    tpFloat p0_2 = tpVec2LengthSquared(_p0);
    tpFloat p0p1 = tpVec2Dot(_p0, _p1);
    tpVec2 p0_p1 = tpVec2Sub(_p0, _p1);
    tpFloat p0_p1_2 = tpVec2LengthSquared(p0_p1);
    /* quadratic equation: a = (p0-p1)^2, b = 2 t (p0.p1 - p0^2), c = p0^2 -
     * 1 */
    tpFloat discriminant = (p0p1 - p0_2) * (p0p1 - p0_2) - p0_p1_2 * (p0_2 - 1);
    assert(discriminant >= 0);
    return (sqrt(discriminant) - p0p1 + p0_2) / p0_p1_2;
}

TARP_LOCAL tpFloat _tpLerp(tpFloat _a, tpFloat _b, tpFloat _t)
{
    return _a * (1.0f - _t) + _b * _t;
}

TARP_LOCAL tpFloat _tpInvLerp(tpFloat _a, tpFloat _b, tpFloat _x)
{
    assert(_b - _a != 0);
    return (_x - _a) / (_b - _a);
}

TARP_LOCAL void _tpGLGradientRadialGeometry(_tpGLContext * _ctx,
                                            _tpGLGradient * _grad,
                                            const tpTransform * _paintTransform,
                                            const _tpGLRect * _bounds,
                                            tpBool _bIsScalingStroke,
                                            _tpGLTextureVertexArray * _vertices,
                                            int * _outVertexOffset,
                                            int * _outVertexCount)
{
    /* regenerate the geometry for this path/gradient combo */
    _tpGLTextureVertex vertices[TARP_RADIAL_GRADIENT_SLICES + 7];
    int vertexCount;
    tpTransform ellipse, inverse;
    tpMat2 rot;
    tpVec2 focalPoint, a, b, circleFocalPoint;
    tpVec2 tmp, tmp2, tmp3;
    tpFloat phi, t;

    /* apply _paintTransform */
    a = tpVec2Sub(_grad->destination, _grad->origin);
    b.x = -a.y * _grad->ratio;
    b.y = a.x * _grad->ratio;
    ellipse = tpTransformMake(a.x, b.x, _grad->origin.x, a.y, b.y, _grad->origin.y);
    ellipse = tpTransformCombine(_paintTransform, &ellipse);

    if (!_bIsScalingStroke)
        ellipse = tpTransformCombine(&_ctx->transform, &ellipse);

    a = tpVec2Make(ellipse.m.v[0], ellipse.m.v[1]);
    b = tpVec2Make(ellipse.m.v[2], ellipse.m.v[3]);
    if (tpVec2Cross(a, b) < 0)
    {
        ellipse.m.v[2] = -ellipse.m.v[2];
        ellipse.m.v[3] = -ellipse.m.v[3];
    }

    tmp = tpVec2Add(_grad->focal_point_offset, _grad->origin);
    focalPoint = tpTransformApply(_paintTransform, tmp);

    if (!_bIsScalingStroke)
        focalPoint = tpTransformApply(&_ctx->transform, focalPoint);

    /* avoid numerical instabilities for gradients of near-zero size */
    /* @TODO: The values of 0.1 are somewhat arbitrarily chosen. This might
     * require more rigorous analysis. */
    if (tpVec2LengthSquared(a) < 0.1 || tpVec2LengthSquared(b) < 0.1 ||
        fabs(tpVec2Cross(a, b)) < 0.1)
    {
        vertices[0].vertex = _bounds->min;
        vertices[1].vertex = tpVec2Make(_bounds->max.x, _bounds->min.y);
        vertices[2].vertex = _bounds->max;
        vertices[3].vertex = tpVec2Make(_bounds->min.x, _bounds->max.y);
        vertices[0].tc = vertices[1].tc = vertices[2].tc = vertices[3].tc = tpVec2Make(1, 0);
        *_outVertexOffset = _vertices->count;
        *_outVertexCount = 4;
        _tpGLTextureVertexArrayAppendCArray(_vertices, vertices, 4);
        return;
    }

    /* find a transform, which converts the ellipse into a unit circle */
    inverse = tpTransformInvert(&ellipse);

    /* ensure that the focal point lies inside the ellipse */
    tmp = tpTransformApply(&inverse, focalPoint);
    t = tpVec2Length(tmp);
    /* @TODO: Proper epsilon */
    if (t > 0.999f)
    {
        tmp2 = tpVec2MultScalar(tmp, 0.999f / t);
        focalPoint = tpTransformApply(&ellipse, tmp2);
    }
    circleFocalPoint = tpTransformApply(&inverse, focalPoint);

    vertices[0].vertex = focalPoint;
    vertices[0].tc = tpVec2Make(0, 0);
    vertexCount = 1;

    phi = 2 * TARP_PI / TARP_RADIAL_GRADIENT_SLICES;
    rot = tpMat2MakeRotation(phi);

    /* max x, min y corner */
    tmp2 = tpVec2Make(_bounds->max.x, _bounds->min.y);
    vertices[vertexCount].vertex = tmp2;
    tmp3 = tpTransformApply(&inverse, tmp2);
    t = _tpLineUnitCircleIntersection(circleFocalPoint, tmp3);
    tmp = tpVec2Lerp(circleFocalPoint, tmp3, t);
    vertices[vertexCount].tc.x = 1 / t;
    ++vertexCount;

    /* max x edge */
    if (focalPoint.x < _bounds->max.x)
        for (;;)
        {
            tmp = tpMat2MultVec2(&rot, tmp);
            tmp2 = tpTransformApply(&ellipse, tmp);
            t = _tpInvLerp(focalPoint.x, tmp2.x, _bounds->max.x);
            tmp3 = tpVec2Make(_bounds->max.x, _tpLerp(focalPoint.y, tmp2.y, t));
            if (tmp2.x <= focalPoint.x || tmp3.y > _bounds->max.y)
            {
                break;
            }
            vertices[vertexCount].vertex = tmp3;
            vertices[vertexCount].tc.x = t;
            ++vertexCount;
        }

    /* max x, max y corner */
    tmp2 = _bounds->max;
    vertices[vertexCount].vertex = tmp2;
    tmp3 = tpTransformApply(&inverse, tmp2);
    t = _tpLineUnitCircleIntersection(circleFocalPoint, tmp3);
    tmp = tpVec2Lerp(circleFocalPoint, tmp3, t);
    vertices[vertexCount].tc.x = 1 / t;
    ++vertexCount;

    /* max y edge */
    if (focalPoint.y < _bounds->max.y)
        for (;;)
        {
            tmp = tpMat2MultVec2(&rot, tmp);
            tmp2 = tpTransformApply(&ellipse, tmp);
            t = _tpInvLerp(focalPoint.y, tmp2.y, _bounds->max.y);
            tmp3 = tpVec2Make(_tpLerp(focalPoint.x, tmp2.x, t), _bounds->max.y);
            if (tmp2.y <= focalPoint.y || tmp3.x < _bounds->min.x)
            {
                break;
            }
            vertices[vertexCount].vertex = tmp3;
            vertices[vertexCount].tc.x = t;
            ++vertexCount;
        }

    /* min x, max y corner */
    tmp2 = tpVec2Make(_bounds->min.x, _bounds->max.y);
    vertices[vertexCount].vertex = tmp2;
    tmp3 = tpTransformApply(&inverse, tmp2);
    t = _tpLineUnitCircleIntersection(circleFocalPoint, tmp3);
    tmp = tpVec2Lerp(circleFocalPoint, tmp3, t);
    vertices[vertexCount].tc.x = 1 / t;
    ++vertexCount;

    /* min x edge */
    if (focalPoint.x > _bounds->min.x)
    {
        for (;;)
        {
            tmp = tpMat2MultVec2(&rot, tmp);
            tmp2 = tpTransformApply(&ellipse, tmp);
            t = _tpInvLerp(focalPoint.x, tmp2.x, _bounds->min.x);
            tmp3 = tpVec2Make(_bounds->min.x, _tpLerp(focalPoint.y, tmp2.y, t));
            if (tmp2.x >= focalPoint.x || tmp3.y < _bounds->min.y)
            {
                break;
            }
            vertices[vertexCount].vertex = tmp3;
            vertices[vertexCount].tc.x = t;
            ++vertexCount;
        }
    }

    /* min x, min y corner */
    tmp2 = _bounds->min;
    vertices[vertexCount].vertex = tmp2;
    tmp3 = tpTransformApply(&inverse, tmp2);
    t = _tpLineUnitCircleIntersection(circleFocalPoint, tmp3);
    tmp = tpVec2Lerp(circleFocalPoint, tmp3, t);
    vertices[vertexCount].tc.x = 1 / t;
    ++vertexCount;

    /* min y edge */
    if (focalPoint.y > _bounds->min.y)
    {
        for (;;)
        {
            tmp = tpMat2MultVec2(&rot, tmp);
            tmp2 = tpTransformApply(&ellipse, tmp);
            t = _tpInvLerp(focalPoint.y, tmp2.y, _bounds->min.y);
            tmp3 = tpVec2Make(_tpLerp(focalPoint.x, tmp2.x, t), _bounds->min.y);
            if (tmp2.y >= focalPoint.y || tmp3.x > _bounds->max.x)
            {
                break;
            }
            vertices[vertexCount].vertex = tmp3;
            vertices[vertexCount].tc.x = t;
            ++vertexCount;
        }
    }

    /* max x, min y corner */
    vertices[vertexCount].vertex = vertices[1].vertex;
    vertices[vertexCount].tc.x = vertices[1].tc.x;
    ++vertexCount;

    assert((unsigned long)vertexCount <= sizeof(vertices) / sizeof(_tpGLTextureVertex));

    *_outVertexOffset = _vertices->count;
    *_outVertexCount = vertexCount;
    _tpGLTextureVertexArrayAppendCArray(_vertices, vertices, vertexCount);
}

TARP_LOCAL void _tpGLCacheGradientGeometry(_tpGLContext * _ctx,
                                           _tpGLGradient * _grad,
                                           _tpGLTextureVertexArray * _oldVertices,
                                           _tpGLGradientCacheData * _gradCache,
                                           _tpGLTextureVertexArray * _outVertices,
                                           const tpTransform * _paintTransform,
                                           tpBool _bIsScalingStroke)
{
    _tpGLGradient * grad = _grad;

    /* check if we need to update the gradient texture */
    if (grad->bDirty)
    {
        grad->bDirty = tpFalse;
        /* ensure that the color stops are valid/complete */
        _tpGLFinalizeColorStops(_ctx, grad);
        /* update the ramp texture */
        /* TODO: update the ramp texture separately just before drawing to
         * avoid multiple texture binds */
        _tpGLUpdateRampTexture(grad);
        _oldVertices = NULL;
    }

    /* rebuild the gradient geometry if the gradient is dirty, different to the last one
        the path used, or there are no pre cached _vertices */
    if (!_oldVertices)
    {
        /* rebuild the gradient */
        if (grad->type == kTpGradientTypeLinear)
        {
            _tpGLGradientLinearGeometry(_ctx,
                                        grad,
                                        _paintTransform,
                                        _gradCache->bounds,
                                        _bIsScalingStroke,
                                        _outVertices,
                                        &_gradCache->vertexOffset,
                                        &_gradCache->vertexCount);
        }
        else if (grad->type == kTpGradientTypeRadial)
        {
            _tpGLGradientRadialGeometry(_ctx,
                                        grad,
                                        _paintTransform,
                                        _gradCache->bounds,
                                        _bIsScalingStroke,
                                        _outVertices,
                                        &_gradCache->vertexOffset,
                                        &_gradCache->vertexCount);
        }
    }
    else
    {
        /* copy cached gradient data */
        _tpGLTextureVertexArrayAppendCArray(
            _outVertices,
            _tpGLTextureVertexArrayAtPtr(_oldVertices, _gradCache->vertexOffset),
            _gradCache->vertexCount);
    }
}

TARP_API tpBool tpPrepareDrawing(tpContext _ctx)
{
    _tpGLContext * ctx = (_tpGLContext *)_ctx.pointer;

    /* cache previous render state so we can reset it in tpFinishDrawing */
    glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint *)&ctx->stateBackup.activeTexture);
    ctx->stateBackup.depthTest = glIsEnabled(GL_DEPTH_TEST);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &ctx->stateBackup.depthMask);
    ctx->stateBackup.multisample = glIsEnabled(GL_MULTISAMPLE);
    ctx->stateBackup.stencilTest = glIsEnabled(GL_STENCIL_TEST);
    glGetIntegerv(GL_STENCIL_WRITEMASK, (GLint *)&ctx->stateBackup.stencilMask);
    glGetIntegerv(GL_STENCIL_FAIL, (GLint *)&ctx->stateBackup.stencilFail);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, (GLint *)&ctx->stateBackup.stencilPassDepthPass);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, (GLint *)&ctx->stateBackup.stencilPassDepthFail);
    glGetIntegerv(GL_STENCIL_CLEAR_VALUE, (GLint *)&ctx->stateBackup.clearStencil);
    ctx->stateBackup.blending = glIsEnabled(GL_BLEND);
    glGetIntegerv(GL_BLEND_SRC_RGB, (GLint *)&ctx->stateBackup.blendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, (GLint *)&ctx->stateBackup.blendDestRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint *)&ctx->stateBackup.blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint *)&ctx->stateBackup.blendDestAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint *)&ctx->stateBackup.blendEquationRGB);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint *)&ctx->stateBackup.blendEquationAlpha);
    ctx->stateBackup.cullFace = glIsEnabled(GL_CULL_FACE);
    glGetIntegerv(GL_CULL_FACE_MODE, (GLint *)&ctx->stateBackup.cullFaceMode);
    glGetIntegerv(GL_FRONT_FACE, (GLint *)&ctx->stateBackup.frontFace);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint *)&ctx->stateBackup.vao);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint *)&ctx->stateBackup.vbo);
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *)&ctx->stateBackup.program);

    _TARP_ASSERT_NO_GL_ERROR(glActiveTexture(GL_TEXTURE0));

    _TARP_ASSERT_NO_GL_ERROR(glDisable(GL_DEPTH_TEST));
    _TARP_ASSERT_NO_GL_ERROR(glDepthMask(GL_FALSE));
    _TARP_ASSERT_NO_GL_ERROR(glEnable(GL_MULTISAMPLE));

    /* @TODO: Support different ways of blending?? */
    _TARP_ASSERT_NO_GL_ERROR(glEnable(GL_BLEND));
    _TARP_ASSERT_NO_GL_ERROR(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    _TARP_ASSERT_NO_GL_ERROR(glEnable(GL_STENCIL_TEST));
    _TARP_ASSERT_NO_GL_ERROR(
        glStencilMask(_kTpGLFillRasterStencilPlane | _kTpGLClippingStencilPlaneOne |
                      _kTpGLClippingStencilPlaneTwo | _kTpGLStrokeRasterStencilPlane));
    _TARP_ASSERT_NO_GL_ERROR(glClearStencil(0));
    _TARP_ASSERT_NO_GL_ERROR(glClear(GL_STENCIL_BUFFER_BIT));

    _TARP_ASSERT_NO_GL_ERROR(glBindVertexArray(ctx->vao.vao));
    _TARP_ASSERT_NO_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, ctx->vao.vbo));

    _TARP_ASSERT_NO_GL_ERROR(glUseProgram(ctx->program));

    ctx->bCanSwapStencilPlanes = tpTrue;
    ctx->currentClipStencilPlane = _kTpGLClippingStencilPlaneOne;
    ctx->clippingStackDepth = 0; /* reset clipping */

    return tpFalse;
}

TARP_API tpBool tpFinishDrawing(tpContext _ctx)
{
    /* reset gl state to what it was before we began drawing */
    _tpGLContext * ctx = (_tpGLContext *)_ctx.pointer;

    /* we dont assert gl errors here for now...should we? */
    glActiveTexture(ctx->stateBackup.activeTexture);
    ctx->stateBackup.depthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    glDepthMask(ctx->stateBackup.depthMask);
    ctx->stateBackup.multisample ? glEnable(GL_MULTISAMPLE) : glDisable(GL_MULTISAMPLE);
    ctx->stateBackup.stencilTest ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
    glStencilMask(ctx->stateBackup.stencilMask);
    glStencilOp(ctx->stateBackup.stencilFail,
                ctx->stateBackup.stencilPassDepthPass,
                ctx->stateBackup.stencilPassDepthFail);
    glClearStencil(ctx->stateBackup.clearStencil);
    ctx->stateBackup.blending ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    glBlendFuncSeparate(ctx->stateBackup.blendSrcRGB,
                        ctx->stateBackup.blendDestRGB,
                        ctx->stateBackup.blendSrcAlpha,
                        ctx->stateBackup.blendDestAlpha);
    glBlendEquationSeparate(ctx->stateBackup.blendEquationRGB, ctx->stateBackup.blendEquationAlpha);

    ctx->stateBackup.cullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    glCullFace(ctx->stateBackup.cullFaceMode);
    glFrontFace(ctx->stateBackup.frontFace);
    glBindVertexArray(ctx->stateBackup.vao);
    glBindBuffer(GL_ARRAY_BUFFER, ctx->stateBackup.vbo);
    glUseProgram(ctx->stateBackup.program);

    return tpFalse;
}

TARP_LOCAL void _tpGLPrepareStencilPlanes(_tpGLContext * _ctx,
                                          tpBool _bIsClippingPath,
                                          int * _outTargetStencilPlane,
                                          int * _outTestStencilPlane)
{
    *_outTargetStencilPlane =
        _bIsClippingPath ? _ctx->currentClipStencilPlane : _kTpGLFillRasterStencilPlane;
    *_outTestStencilPlane = _ctx->currentClipStencilPlane == _kTpGLClippingStencilPlaneOne
                                ? _kTpGLClippingStencilPlaneTwo
                                : _kTpGLClippingStencilPlaneOne;
}

TARP_API tpBool _tpGLCachePathImpl(_tpGLContext * _ctx,
                                   _tpGLPath * _path,
                                   const tpStyle * _style,
                                   _tpGLRenderCache * _oldCache,
                                   _tpGLRenderCache * _cache,
                                   tpBool _bGeometryDirty,
                                   tpBool _bStrokeDirty,
                                   tpBool _bFillGradientDirty,
                                   tpBool _bStrokeGradientDirty)
{
    _tpGLRect bounds;
    tpBool bStyleHasStroke, bIsPathRenderCache;
    int i;

    assert(_ctx && _path && _cache);

    /* if the rendercache is currently referenced in the clipping stack of the context, create a
     * deep copy and replace it in the clipping stack. This ensures that the clipping masks will be
     * identical in cases where the clipping stack needs to be rebuilt. */
    for (i = 0; i < _ctx->clippingStackDepth; ++i)
    {
        if (_ctx->clippingStack[i] == _cache)
        {
            _tpGLRenderCache * c = _ctx->clippingRenderCaches[i];
            _tpGLRenderCacheCopyTo(_cache, c);
            _ctx->clippingStack[i] = c;
        }
    }

    /* simpliy reset the cache if the path is empty */
    if (!_path->contours.count)
    {
        _tpGLRenderCacheContourArrayClear(&_cache->contours);
        _tpVec2ArrayClear(&_cache->geometryCache);
        _tpBoolArrayClear(&_cache->jointCache);
        _tpGLTextureVertexArrayClear(&_cache->textureGeometryCache);
        return tpFalse;
    }

    bStyleHasStroke = (tpBool)(_style->stroke.type != kTpPaintTypeNone && _style->strokeWidth > 0);
    bIsPathRenderCache = (tpBool)(_path->renderCache == _cache);
    _tpGLInitBounds(&bounds);
    _tpGLRenderCacheCopyStyle(_style, _cache);

    /* make sure transformProjection is up to date */
    if (_ctx->bTransformProjDirty)
    {
        _ctx->bTransformProjDirty = tpFalse;
        _ctx->renderTransform = tpMat4MakeFrom2DTransform(&_ctx->transform);
        _ctx->transformProjection = tpMat4Mult(&_ctx->projection, &_ctx->renderTransform);
    }

    /* cache the matrix that should be used during rendering */
    if (!_style->scaleStroke)
        _cache->renderMatrix = _ctx->projection;
    else
        _cache->renderMatrix = _ctx->transformProjection;

    if (_bGeometryDirty)
    {
        int i;
        _tpGLContour * c;
        _tpGLRenderCacheContour rc;
        tpFloat angleTolerance = !_style->scaleStroke ? 0.15f : 0.15f / _ctx->transformScale;

        for (i = 0; i < _path->contours.count; ++i)
        {
            c = _tpGLContourArrayAtPtr(&_path->contours, i);
            if (c->bDirty || !_oldCache->contours.count)
            {
                if (_style->scaleStroke)
                    _tpGLFlattenContour(c,
                                        angleTolerance,
                                        NULL,
                                        &_ctx->tmpVertices,
                                        &_ctx->tmpJoints,
                                        &bounds,
                                        &rc);
                else
                    _tpGLFlattenContour(c,
                                        angleTolerance,
                                        &_ctx->transform,
                                        &_ctx->tmpVertices,
                                        &_ctx->tmpJoints,
                                        &bounds,
                                        &rc);

                /* only clear the dirty flag here, if there is no stroke, otherwise this will be
                 * done during stroking. We also ignore the dirty flag if this is not the internal
                 * path render cache */
                if (!bStyleHasStroke && bIsPathRenderCache)
                    c->bDirty = tpFalse;
            }
            else
            {
                _tpGLRenderCacheContour * rcc =
                    _tpGLRenderCacheContourArrayAtPtr(&_oldCache->contours, i);

                rc.fillVertexOffset = _ctx->tmpVertices.count;
                rc.fillVertexCount = rcc->fillVertexCount;
                rc.bIsClosed = c->bIsClosed;

                /* otherwise we just copy the contour to the tmpbuffer... */
                _tpVec2ArrayAppendCArray(
                    &_ctx->tmpVertices,
                    _tpVec2ArrayAtPtr(&_oldCache->geometryCache, rcc->fillVertexOffset),
                    rcc->fillVertexCount);

                _tpBoolArrayAppendCArray(
                    &_ctx->tmpJoints,
                    _tpBoolArrayAtPtr(&_oldCache->jointCache, rcc->fillVertexOffset),
                    rcc->fillVertexCount);

                /* ...and merge the cached contour bounds with the path bounds
                 */
                _tpGLMergeBounds(&bounds, &c->bounds);
            }

            _tpGLRenderCacheContourArrayAppendPtr(&_ctx->tmpRcContours, &rc);
        }

        /* generate and add the stroke geometry to the tmp buffers */
        if (_style->stroke.type != kTpPaintTypeNone && _style->strokeWidth > 0)
        {
            _cache->strokeVertexOffset = _ctx->tmpVertices.count;
            _cache->strokeVertexCount = 0;
            _tpGLStroke(_path,
                        &_ctx->tmpRcContours,
                        bIsPathRenderCache,
                        _bStrokeDirty ? NULL : _oldCache,
                        _style,
                        &_ctx->tmpVertices,
                        &_ctx->tmpJoints,
                        &_cache->strokeVertexCount);
        }

        /* save the path bounds */
        _cache->boundsCache = bounds;

        /* swap the tmp buffers with the path caches */
        _tpGLRenderCacheContourArrayClear(&_cache->contours);
        _tpVec2ArrayClear(&_cache->geometryCache);
        _tpBoolArrayClear(&_cache->jointCache);
        _tpGLRenderCacheContourArraySwap(&_cache->contours, &_ctx->tmpRcContours);
        _tpVec2ArraySwap(&_cache->geometryCache, &_ctx->tmpVertices);
        _tpBoolArraySwap(&_cache->jointCache, &_ctx->tmpJoints);

        /* add the bounds geometry to the geom cache (and potentially cache
         * stroke bounds) */
        _tpGLCacheBoundsGeometry(_cache, _style);

        /* we are done with the stroke allready... */
        _bStrokeDirty = tpFalse;
    }

    if (_bStrokeDirty)
    {
        /* remove all the old stroke vertices from the cache if this is not a full geometry
        update*/

        /* the stroke was possibly removed, in that case this is enough */
        _cache->strokeVertexCount = 0;

        if (bStyleHasStroke)
        {
            /* otherwise check if there was a previous stroke. If so, remove it along with the
             * cached bounds geometry */
            if (_cache->strokeVertexOffset)
                _tpVec2ArrayRemoveRange(&_cache->geometryCache,
                                        _cache->strokeVertexOffset,
                                        _cache->geometryCache.count);
            else
                /* otherwise just remove the cached bounds geometry */
                _tpVec2ArrayRemoveRange(&_cache->geometryCache,
                                        _cache->geometryCache.count - 4,
                                        _cache->geometryCache.count);

            /* generate and add the stroke geometry to the cache. */
            _cache->strokeVertexOffset = _cache->geometryCache.count;
            _cache->strokeVertexCount = 0;
            _tpGLStroke(_path,
                        &_cache->contours,
                        bIsPathRenderCache,
                        NULL,
                        _style,
                        &_cache->geometryCache,
                        &_cache->jointCache,
                        &_cache->strokeVertexCount);

            /* add the stroke geometry to the cache. */
            _tpGLCacheBoundsGeometry(_cache, _style);

            /* force rebuilding of the stroke gradient geometry */
            _bStrokeGradientDirty = tpTrue;
        }
    }

    if (_bFillGradientDirty || _bStrokeGradientDirty)
    {
        _tpGLTextureVertexArrayClear(&_ctx->tmpTexVertices);
        if (_style->fill.type == kTpPaintTypeGradient)
        {
            _tpGLGradient * grad = (_tpGLGradient *)_style->fill.data.gradient.pointer;
            _tpGLCacheGradientGeometry(_ctx,
                                       grad,
                                       _bFillGradientDirty ? NULL
                                                           : &_oldCache->textureGeometryCache,
                                       &_cache->fillGradientData,
                                       &_ctx->tmpTexVertices,
                                       &_path->fillPaintTransform,
                                       _style->scaleStroke);
        }

        if (_style->stroke.type == kTpPaintTypeGradient)
        {
            _tpGLGradient * grad = (_tpGLGradient *)_style->stroke.data.gradient.pointer;
            _tpGLCacheGradientGeometry(_ctx,
                                       grad,
                                       _bStrokeGradientDirty ? NULL
                                                             : &_oldCache->textureGeometryCache,
                                       &_cache->strokeGradientData,
                                       &_ctx->tmpTexVertices,
                                       &_path->strokePaintTransform,
                                       _style->scaleStroke);
        }

        _tpGLTextureVertexArraySwap(&_cache->textureGeometryCache, &_ctx->tmpTexVertices);
    }

    return tpFalse;
}

TARP_API tpBool tpCachePath(tpContext _ctx,
                            tpPath _path,
                            const tpStyle * _style,
                            tpRenderCache _cache)
{
    _tpGLPath * path = (_tpGLPath *)_path.pointer;
    return _tpGLCachePathImpl((_tpGLContext *)_ctx.pointer,
                              path,
                              _style,
                              path->renderCache,
                              (_tpGLRenderCache *)_cache.pointer,
                              tpTrue,
                              tpTrue,
                              tpTrue,
                              tpTrue);
}

TARP_LOCAL tpBool _tpGLDrawRenderCacheImpl(_tpGLContext * _ctx,
                                           _tpGLRenderCache * _cache,
                                           tpBool _bIsClipPath)
{
    GLint i;
    GLuint stencilPlaneToWriteTo, stencilPlaneToTestAgainst;

    if (!_cache->contours.count)
        return tpFalse;

    assert(_cache->geometryCache.count);

    if (!_bIsClipPath && (_cache->style.fill.type == kTpPaintTypeGradient ||
                          _cache->style.stroke.type == kTpPaintTypeGradient))
    {
        _TARP_ASSERT_NO_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, _ctx->textureVao.vbo));
        _tpGLUpdateVAO(&_ctx->textureVao,
                       _cache->textureGeometryCache.array,
                       sizeof(_tpGLTextureVertex) * _cache->textureGeometryCache.count);
        _TARP_ASSERT_NO_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, _ctx->vao.vbo));
    }

    /* upload the paths geometry cache to the gpu */
    _tpGLUpdateVAO(
        &_ctx->vao, _cache->geometryCache.array, sizeof(tpVec2) * _cache->geometryCache.count);

    _TARP_ASSERT_NO_GL_ERROR(
        glUniformMatrix4fv(_ctx->tpLoc, 1, GL_FALSE, &_cache->renderMatrix.v[0]));

    /* draw the fill */
    stencilPlaneToWriteTo =
        _bIsClipPath ? _ctx->currentClipStencilPlane : _kTpGLFillRasterStencilPlane;
    stencilPlaneToTestAgainst = _ctx->currentClipStencilPlane == _kTpGLClippingStencilPlaneOne
                                    ? _kTpGLClippingStencilPlaneTwo
                                    : _kTpGLClippingStencilPlaneOne;

    if (_bIsClipPath || _cache->style.fill.type != kTpPaintTypeNone)
    {
        _TARP_ASSERT_NO_GL_ERROR(glStencilFunc(
            _ctx->clippingStackDepth ? GL_EQUAL : GL_ALWAYS, 0, stencilPlaneToTestAgainst));
        _TARP_ASSERT_NO_GL_ERROR(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));
        if (_cache->style.fillRule == kTpFillRuleEvenOdd)
        {
            _TARP_ASSERT_NO_GL_ERROR(glStencilMask(stencilPlaneToWriteTo));
            _TARP_ASSERT_NO_GL_ERROR(glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT));

            for (i = 0; i < _cache->contours.count; ++i)
            {
                _tpGLRenderCacheContour * c =
                    _tpGLRenderCacheContourArrayAtPtr(&_cache->contours, i);

                _TARP_ASSERT_NO_GL_ERROR(
                    glDrawArrays(GL_TRIANGLE_FAN, c->fillVertexOffset, c->fillVertexCount));
            }

            if (_bIsClipPath)
            {
                _TARP_ASSERT_NO_GL_ERROR(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
                return tpFalse;
            }

            _TARP_ASSERT_NO_GL_ERROR(glStencilFunc(GL_NOTEQUAL, 0, _kTpGLFillRasterStencilPlane));
        }
        else if (_cache->style.fillRule == kTpFillRuleNonZero)
        {
            /*
            NonZero winding rule needs to use Increment and Decrement
            stencil operations. we therefore render to the rasterize mask,
            even if this is a clipping mask, and transfer the results to the
            clipping mask stencil plane afterwards
            */
            _TARP_ASSERT_NO_GL_ERROR(glStencilMask(_kTpGLFillRasterStencilPlane));
            _TARP_ASSERT_NO_GL_ERROR(glStencilOp(GL_KEEP, GL_KEEP, GL_INCR_WRAP));
            _TARP_ASSERT_NO_GL_ERROR(glEnable(GL_CULL_FACE));
            _TARP_ASSERT_NO_GL_ERROR(glCullFace(GL_BACK));
            _TARP_ASSERT_NO_GL_ERROR(glFrontFace(GL_CW));

            for (i = 0; i < _cache->contours.count; ++i)
            {
                _tpGLRenderCacheContour * c =
                    _tpGLRenderCacheContourArrayAtPtr(&_cache->contours, i);
                _TARP_ASSERT_NO_GL_ERROR(
                    glDrawArrays(GL_TRIANGLE_FAN, c->fillVertexOffset, c->fillVertexCount));
            }

            _TARP_ASSERT_NO_GL_ERROR(glFrontFace(GL_CCW));
            _TARP_ASSERT_NO_GL_ERROR(glStencilOp(GL_KEEP, GL_KEEP, GL_DECR_WRAP));

            for (i = 0; i < _cache->contours.count; ++i)
            {
                _tpGLRenderCacheContour * c =
                    _tpGLRenderCacheContourArrayAtPtr(&_cache->contours, i);
                _TARP_ASSERT_NO_GL_ERROR(
                    glDrawArrays(GL_TRIANGLE_FAN, c->fillVertexOffset, c->fillVertexCount));
            }

            _TARP_ASSERT_NO_GL_ERROR(glDisable(GL_CULL_FACE));
            _TARP_ASSERT_NO_GL_ERROR(glFrontFace(GL_CW));

            if (_bIsClipPath)
            {
                _TARP_ASSERT_NO_GL_ERROR(glStencilMask(stencilPlaneToWriteTo));
                _TARP_ASSERT_NO_GL_ERROR(
                    glStencilFunc(GL_NOTEQUAL, 0, _kTpGLFillRasterStencilPlane));
                _TARP_ASSERT_NO_GL_ERROR(glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO));

                _TARP_ASSERT_NO_GL_ERROR(
                    glDrawArrays(GL_TRIANGLE_STRIP, _cache->boundsVertexOffset, 4));

                /*
                draw the bounds one last time to zero out the tmp data
                created in the _kTpGLFillRasterStencilPlane
                */
                _TARP_ASSERT_NO_GL_ERROR(glStencilMask(_kTpGLFillRasterStencilPlane));
                _TARP_ASSERT_NO_GL_ERROR(glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO));
                _TARP_ASSERT_NO_GL_ERROR(
                    glDrawArrays(GL_TRIANGLE_STRIP, _cache->boundsVertexOffset, 4));

                _TARP_ASSERT_NO_GL_ERROR(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

                return tpFalse;
            }
            else
            {
                _TARP_ASSERT_NO_GL_ERROR(
                    glStencilFunc(GL_NOTEQUAL, 0, _kTpGLFillRasterStencilPlane));
            }
        }

        _TARP_ASSERT_NO_GL_ERROR(glStencilMask(_kTpGLFillRasterStencilPlane));
        _TARP_ASSERT_NO_GL_ERROR(glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO));
        _TARP_ASSERT_NO_GL_ERROR(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

        _tpGLDrawPaint(_ctx, _cache, &_cache->style.fill, &_cache->fillGradientData);
    }

    /* we don't care for stroke if this is a clipping path */
    if (_bIsClipPath)
        return tpFalse;

    /* draw the stroke */
    if (_cache->strokeVertexCount)
    {
        _TARP_ASSERT_NO_GL_ERROR(glStencilMask(_kTpGLStrokeRasterStencilPlane));
        _TARP_ASSERT_NO_GL_ERROR(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));
        /* @TODO: Not sure if this test will work with non 8 bit stencil
         * buffers, but is there such a thing oO? */
        _TARP_ASSERT_NO_GL_ERROR(glStencilFunc(
            _ctx->clippingStackDepth ? GL_NOTEQUAL : GL_ALWAYS, 0xff, stencilPlaneToTestAgainst));
        _TARP_ASSERT_NO_GL_ERROR(glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE));

        /* Draw all stroke triangles of all contours at once */
        _TARP_ASSERT_NO_GL_ERROR(
            glDrawArrays(GL_TRIANGLES, _cache->strokeVertexOffset, _cache->strokeVertexCount));

        _TARP_ASSERT_NO_GL_ERROR(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
        _TARP_ASSERT_NO_GL_ERROR(glStencilFunc(GL_NOTEQUAL, 0, _kTpGLStrokeRasterStencilPlane));
        _TARP_ASSERT_NO_GL_ERROR(glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT));

        _tpGLDrawPaint(_ctx, _cache, &_cache->style.stroke, &_cache->strokeGradientData);
    }

    /* WE DONE BABY */
    return tpFalse;
}

TARP_LOCAL tpBool _tpGLUpdateInternalPathCache(_tpGLContext * _ctx,
                                               _tpGLPath * _path,
                                               const tpStyle * _style,
                                               tpBool _bIsClipPath)
{
    tpBool bGeometryDirty, bStrokeDirty, bFillGradientDirty, bStrokeGradientDirty,
        bMarkAllContoursDirty, bVirgin, bTransformDirty;
    _tpGLRenderCache * cache;

    /* early out if the path has no contours */
    if (!_path->contours.count)
        return tpFalse;

    bVirgin = (tpBool)(_path->renderCache == NULL);
    cache = _tpGLPathEnsureRenderCache(_path);

    if (!bVirgin)
    {
        /* this block of code checks what part of the geometry/path are dirty compared to its cached
         * version */

        bGeometryDirty = _path->bPathGeometryDirty;
        bStrokeDirty = tpFalse;
        bFillGradientDirty = tpFalse;
        bStrokeGradientDirty = tpFalse;
        bMarkAllContoursDirty = tpFalse;
        bTransformDirty = tpFalse;

        /* check if the transform projection is dirty */
        if (_path->lastDrawContext != _ctx || _path->lastTransformID != _ctx->transformID)
        {
            bTransformDirty = tpTrue;

            /* @TODO: we should also take skew into account here, not only scale
             * @TODO: Proper epsilon
             */
            if (_ctx->transformScale - _path->lastTransformScale > 0.001f || !_style->scaleStroke)
            {
                bMarkAllContoursDirty = tpTrue;
            }

            _path->lastTransformID = _ctx->transformID;
            _path->lastDrawContext = _ctx;
            _path->lastTransformScale = _ctx->transformScale;
        }

        /*
        if this style has a stroke and its scale stroke property is different
        from the last style, we force a full reflattening of all path contours.
        we also do this if the style has non scaling stroke and the transform
        changed since we last drew the path.
        */
        if (bMarkAllContoursDirty ||
            (!_path->bPathGeometryDirty && _style->stroke.type != kTpPaintTypeNone &&
             cache->style.scaleStroke != _style->scaleStroke))
        {
            _tpGLPathMarkAllContoursDirty(_path);
            bGeometryDirty = tpTrue;
            bStrokeDirty = tpTrue;
        }

        if (!_bIsClipPath && !bStrokeDirty &&
            (cache->style.stroke.type !=
                 _style->stroke.type || /* @TODO: This could be optimized by checking if either the
                                           last or current stroke type was None and the last and
                                           current type are different (i.e. so if you change from
                                           solid fill to gradient and vice versa the geometry does
                                           not regenerate, which it will right now)*/
             cache->style.strokeWidth != _style->strokeWidth ||
             cache->style.strokeCap != _style->strokeCap ||
             cache->style.strokeJoin != _style->strokeJoin ||
             cache->style.dashCount != _style->dashCount ||
             cache->style.dashOffset != _style->dashOffset ||
             memcmp(cache->style.dashArray,
                    _style->dashArray,
                    sizeof(tpFloat) * _style->dashCount) != 0))
        {
            bStrokeDirty = tpTrue;
        }

        /* TODO: Make this less ugly. We are basically just checking if anything changed that
        would result in a regeneration of the gradient geometry */
        if (!_bIsClipPath &&
            ((_style->fill.type == kTpPaintTypeGradient &&
              (_path->lastFillGradientID !=
                   ((_tpGLGradient *)_style->fill.data.gradient.pointer)->gradientID ||
               _path->bFillPaintTransformDirty ||
               ((_tpGLGradient *)_style->fill.data.gradient.pointer)->bDirty))))
        {
            bFillGradientDirty = tpTrue;
        }
        if (!_bIsClipPath &&
            ((_style->stroke.type == kTpPaintTypeGradient &&
              (_path->lastStrokeGradientID !=
                   ((_tpGLGradient *)_style->stroke.data.gradient.pointer)->gradientID ||
               _path->bStrokePaintTransformDirty ||
               ((_tpGLGradient *)_style->stroke.data.gradient.pointer)->bDirty)) ||
             cache->style.scaleStroke != _style->scaleStroke))
        {
            bStrokeGradientDirty = tpTrue;
        }
    }
    else
    {
        bFillGradientDirty = (tpBool)(_style->fill.type == kTpPaintTypeGradient);
        bStrokeGradientDirty = (tpBool)(_style->stroke.type == kTpPaintTypeGradient);
        bStrokeDirty = bGeometryDirty = tpTrue;
    }

    /* build the cache */
    if (bGeometryDirty || bStrokeDirty || bFillGradientDirty || bStrokeGradientDirty ||
        bTransformDirty)
    {
        if (_tpGLCachePathImpl(_ctx,
                               _path,
                               _style,
                               cache,
                               cache,
                               bGeometryDirty,
                               bStrokeDirty,
                               bFillGradientDirty,
                               bStrokeGradientDirty))
        {
            return tpTrue;
        }

        _path->bPathGeometryDirty = tpFalse;

        /* this is a little ugly...if we recached any gradient, we need to reset the dirty flags and
         * cache the gradient ID. We don't do that in _tpGLCachePathImpl because these things should
         * not be set if a render cache is manually created (i.e. they are only for internal
         * caching) */
        if (bFillGradientDirty)
        {
            _path->bFillPaintTransformDirty = tpFalse;
            _path->lastFillGradientID =
                ((_tpGLGradient *)_style->fill.data.gradient.pointer)->gradientID;
        }
        if (bStrokeGradientDirty)
        {
            _path->bStrokePaintTransformDirty = tpFalse;
            _path->lastStrokeGradientID =
                ((_tpGLGradient *)_style->stroke.data.gradient.pointer)->gradientID;
        }
    }

    return tpFalse;
}

TARP_LOCAL tpBool _tpGLDrawPathImpl(_tpGLContext * _ctx,
                                    _tpGLPath * _path,
                                    const tpStyle * _style,
                                    tpBool _bIsClipPath)
{
    if (_tpGLUpdateInternalPathCache(_ctx, _path, _style, _bIsClipPath))
        return tpTrue;
    /* draw the cache */
    return _tpGLDrawRenderCacheImpl(_ctx, _path->renderCache, _bIsClipPath);
}

TARP_API tpBool tpDrawPath(tpContext _ctx, tpPath _path, const tpStyle * _style)
{
    return _tpGLDrawPathImpl(
        (_tpGLContext *)_ctx.pointer, (_tpGLPath *)_path.pointer, _style, tpFalse);
}

TARP_API tpBool tpDrawRenderCache(tpContext _ctx, tpRenderCache _cache)
{
    return _tpGLDrawRenderCacheImpl(
        (_tpGLContext *)_ctx.pointer, (_tpGLRenderCache *)_cache.pointer, tpFalse);
}

TARP_LOCAL tpBool _tpGLGenerateClippingMaskForRenderCache(_tpGLContext * _ctx,
                                                          _tpGLRenderCache * _cache,
                                                          tpBool _bIsRebuilding)
{
    tpBool drawResult;
    assert(_ctx && _cache);

    if (!_bIsRebuilding)
    {
        _ctx->clippingStack[_ctx->clippingStackDepth++] = _cache;
    }

    /*
    @TODO: Instead of clearing maybe just clear it in endClipping by
    drawing the bounds of the last clip path? could be a potential speed up
    */
    _TARP_ASSERT_NO_GL_ERROR(glStencilMask(_ctx->currentClipStencilPlane));
    _TARP_ASSERT_NO_GL_ERROR(glClearStencil(~0));
    _TARP_ASSERT_NO_GL_ERROR(glClear(GL_STENCIL_BUFFER_BIT));

    /* draw path */
    drawResult = _tpGLDrawRenderCacheImpl(_ctx, _cache, tpTrue);
    if (drawResult)
        return tpTrue;

    _ctx->currentClipStencilPlane = _ctx->currentClipStencilPlane == _kTpGLClippingStencilPlaneOne
                                        ? _kTpGLClippingStencilPlaneTwo
                                        : _kTpGLClippingStencilPlaneOne;

    return tpFalse;
}

TARP_LOCAL tpBool _tpGLGenerateClippingMask(_tpGLContext * _ctx,
                                            _tpGLPath * _path,
                                            tpFillRule _fillRule)
{
    _ctx->clippingStyle.fillRule = _fillRule;
    if (_tpGLUpdateInternalPathCache(_ctx, _path, &_ctx->clippingStyle, tpTrue))
        return tpTrue;

    return _tpGLGenerateClippingMaskForRenderCache(_ctx, _path->renderCache, tpFalse);
}

TARP_API tpBool tpBeginClipping(tpContext _ctx, tpPath _path)
{
    return tpBeginClippingWithFillRule(_ctx, _path, kTpFillRuleEvenOdd);
}

TARP_API tpBool tpBeginClippingWithFillRule(tpContext _ctx, tpPath _path, tpFillRule _rule)
{
    return _tpGLGenerateClippingMask(
        (_tpGLContext *)_ctx.pointer, (_tpGLPath *)_path.pointer, _rule);
}

TARP_API tpBool tpBeginClippingFromRenderCache(tpContext _ctx, tpRenderCache _cache)
{
    return _tpGLGenerateClippingMaskForRenderCache(
        (_tpGLContext *)_ctx.pointer, (_tpGLRenderCache *)_cache.pointer, tpFalse);
}

TARP_API tpBool tpEndClipping(tpContext _ctx)
{
    int i;
    _tpGLContext * ctx = (_tpGLContext *)_ctx.pointer;
    assert(ctx->clippingStackDepth);

    --ctx->clippingStackDepth;

    if (ctx->clippingStackDepth)
    {
        /* check if the last clip mask is still in one of the clipping
         * planes... */
        if (ctx->bCanSwapStencilPlanes)
        {
            ctx->currentClipStencilPlane =
                ctx->currentClipStencilPlane == _kTpGLClippingStencilPlaneOne
                    ? _kTpGLClippingStencilPlaneTwo
                    : _kTpGLClippingStencilPlaneOne;
            ctx->bCanSwapStencilPlanes = tpFalse;
        }
        else
        {
            /* ...otherwise rebuild it */
            ctx->currentClipStencilPlane = _kTpGLClippingStencilPlaneOne;
            ctx->bCanSwapStencilPlanes = tpTrue;
            _TARP_ASSERT_NO_GL_ERROR(
                glStencilMask(_kTpGLClippingStencilPlaneOne | _kTpGLClippingStencilPlaneTwo));
            _TARP_ASSERT_NO_GL_ERROR(glClearStencil(0));
            _TARP_ASSERT_NO_GL_ERROR(glClear(GL_STENCIL_BUFFER_BIT));

            for (i = 0; i < ctx->clippingStackDepth; ++i)
            {
                /* draw clip path */
                _tpGLGenerateClippingMaskForRenderCache(ctx, ctx->clippingStack[i], tpTrue);
            }
        }
    }
    else
    {
        /*
        @TODO: Instead of clearing maybe just redrawing the clipping path
        bounds to reset the stencil? Might scale better for a lot of paths
        :)
        */
        ctx->currentClipStencilPlane = _kTpGLClippingStencilPlaneOne;
        ctx->bCanSwapStencilPlanes = tpTrue;
        _TARP_ASSERT_NO_GL_ERROR(
            glStencilMask(_kTpGLClippingStencilPlaneOne | _kTpGLClippingStencilPlaneTwo));
        _TARP_ASSERT_NO_GL_ERROR(glClearStencil(0));
        _TARP_ASSERT_NO_GL_ERROR(glClear(GL_STENCIL_BUFFER_BIT));
    }

    return tpFalse;
}

TARP_API tpBool tpResetClipping(tpContext _ctx)
{
    _tpGLContext * ctx = (_tpGLContext *)_ctx.pointer;
    _TARP_ASSERT_NO_GL_ERROR(
        glStencilMask(_kTpGLClippingStencilPlaneOne | _kTpGLClippingStencilPlaneTwo));
    _TARP_ASSERT_NO_GL_ERROR(glClearStencil(0));
    _TARP_ASSERT_NO_GL_ERROR(glClear(GL_STENCIL_BUFFER_BIT));

    ctx->currentClipStencilPlane = _kTpGLClippingStencilPlaneOne;
    ctx->bCanSwapStencilPlanes = tpTrue;
    ctx->clippingStackDepth = 0;

    return tpFalse;
}

TARP_API tpBool tpSetProjection(tpContext _ctx, const tpMat4 * _projection)
{
    _tpGLContext * ctx = (_tpGLContext *)_ctx.pointer;
    ctx->projection = *_projection;
    ctx->projectionID++;
    ctx->bTransformProjDirty = tpTrue;

    return tpFalse;
}

TARP_API tpBool tpSetTransform(tpContext _ctx, const tpTransform * _transform)
{
    tpFloat rotation;
    _tpGLContext * ctx = (_tpGLContext *)_ctx.pointer;

    if (!tpTransformEquals(_transform, &ctx->transform))
    {
        tpVec2 scale, skew, translation;
        ctx->transform = *_transform;
        ctx->transformID++;
        ctx->bTransformProjDirty = tpTrue;

        tpTransformDecompose(_transform, &translation, &scale, &skew, &rotation);
        ctx->transformScale = TARP_MAX(scale.x, scale.y);
    }

    return tpFalse;
}

TARP_API tpBool tpResetTransform(tpContext _ctx)
{
    _tpGLContext * ctx = (_tpGLContext *)_ctx.pointer;
    ctx->transform = tpTransformMakeIdentity();
    ctx->transformScale = 1.0;
    ctx->transformID++;
    ctx->bTransformProjDirty = tpTrue;
    return tpFalse;
}

#endif /* TARP_IMPLEMENTATION_OPENGL */
#endif /* TARP_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif

#endif /* TARP_TARP_H */

/*
MIT License:
------------------------------------------------------------------------------
Copyright (c) 2018 Matthias Dörfelt
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
*/
