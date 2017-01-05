// GEOMETRY.CPP
// Copyright (c) A.Sobolev 2007, 2008, 2010
//
#include <slib.h>

float fmin2(float a, float b);
float fmin3(float a, float b, float c);
float fmin4(float a, float b, float c, float d);
float fmax2(float a, float b);
float fmax3(float a, float b, float c);
float fmax4(float a, float b, float c, float d);

class GeomPath;

//==============================================================================
static const float lineMarker         = 100001.0f;
static const float moveMarker         = 100002.0f;
static const float quadMarker         = 100003.0f;
static const float cubicMarker        = 100004.0f;
static const float closePathMarker    = 100005.0f;
static const float closeSubPathMarker = 100005.0f;
static const int   defaultGranularity = 32;
//
// Represents a type of justification to be used when positioning graphical items.
// e.g. it indicates whether something should be placed top-left, top-right, centred, etc.
//
// It is used in various places wherever this kind of information is needed.
//
class Justification {
public:
	//
	// Creates a Justification object using a combination of flags.
	//
	inline Justification(int flags_)
	{
		flags = flags_;
	}
	//
	// Creates a copy of another Justification object.
	//
	Justification(const Justification & other);
	//
	// Copies another Justification object.
	//
	const Justification& operator= (const Justification& other);
	//
	// Returns the raw flags that are set for this Justification object.
	//
	inline int getFlags() const { return flags; }
	//
	// Tests a set of flags for this object.
	// @returns true if any of the flags passed in are set on this object.
	//
	inline bool testFlags(const int flagsToTest) const { return (flags & flagsToTest) != 0; }
	//
	// Returns just the flags from this object that deal with vertical layout.
	//
	int getOnlyVerticalFlags() const;
	//
	// Returns just the flags from this object that deal with horizontal layout.
	//
	int getOnlyHorizontalFlags() const;
	//
	// Adjusts the position of a rectangle to fit it into a space.
	//
	// The (x, y) position of the rectangle will be updated to position it inside the
	// given space according to the justification flags.
	//
	void   applyToRectangle(int& x, int& y, const  int w, const int h, const  int spaceX, const int spaceY, const  int spaceW, const int spaceH) const throw();
	//
	// Flag values that can be combined and used in the constructor.
	//
	enum {
		left  = 1, // Indicates that the item should be aligned against the left edge of the available space.
		right = 2, // Indicates that the item should be aligned against the right edge of the available space.
		horizontallyCentred = 4, // Indicates that the item should be placed in the centre between the
			// left and right sides of the available space.
		top    = 8,  // Indicates that the item should be aligned against the top edge of the available space.
		bottom = 16, //Indicates that the item should be aligned against the bottom edge of the available space.
		verticallyCentred = 32, // Indicates that the item should be placed in the centre between the top
			// and bottom sides of the available space.
		horizontallyJustified = 64, // Indicates that lines of text should be spread out to fill the maximum width
			// available, so that both margins are aligned vertically.

		centred = 36, // Indicates that the item should be centred vertically and horizontally.
			// This is equivalent to (horizontallyCentred | verticallyCentred)
		centredLeft = 33, // Indicates that the item should be centred vertically but placed on the left hand side.
			// This is equivalent to (left | verticallyCentred)
		centredRight = 34, // Indicates that the item should be centred vertically but placed on the right
			// hand side. This is equivalent to (right | verticallyCentred)
		centredTop = 12, // Indicates that the item should be centred horizontally and placed at the top.
			// This is equivalent to (horizontallyCentred | top)
		centredBottom = 20, // Indicates that the item should be centred horizontally and placed at the bottom.
			// This is equivalent to (horizontallyCentred | bottom)
		topLeft = 9, // Indicates that the item should be placed in the top-left corner.
			// This is equivalent to (left | top)
		topRight = 10, // Indicates that the item should be placed in the top-right corner.
			// This is equivalent to (right | top)
		bottomLeft = 17, // Indicates that the item should be placed in the bottom-left corner.
			// This is equivalent to (left | bottom)
		bottomRight = 18 // Indicates that the item should be placed in the bottom-left corner.
			//This is equivalent to (right | bottom)
	};
private:
	int    flags;
};
//
// A path is a sequence of lines and curves that may either form a closed shape or be open-ended.
//
// To use a path, you can create an empty one, then add lines and curves to it
// to create shapes, then it can be rendered by a Graphics context or used for geometric operations.
//
// e.g. @code
// Path myPath;
//
// myPath.StartNewSubPath (10.0f, 10.0f);          // move the current position to (10, 10)
// myPath.lineTo (100.0f, 200.0f);                 // draw a line from here to (100, 200)
// myPath.quadraticTo (0.0f, 150.0f, 5.0f, 50.0f); // draw a curve that ends at (5, 50)
// myPath.closeSubPath();                          // close the subpath with a line back to (10, 10)
// add an ellipse as well, which will form a second sub-path within the path..
// myPath.addEllipse (50.0f, 50.0f, 40.0f, 30.0f);
//
// double the width of the whole thing..
// myPath.applyTransform (AffineTransform::scale (2.0f, 1.0f));
//
// and draw it to a graphics context with a 5-pixel thick outline.
// g.strokePath (myPath, PathStrokeType (5.0f));
//
// @endcode
//
// A path object can actually contain multiple sub-paths, which may themselves be open or closed.
//
// @see PathFlatteningIterator, PathStrokeType, Graphics
//
//
// Represents a 2D affine-transformation matrix.
// An affine transformation is a transformation such as a rotation, scale, shear, resize or translation.
// These are used for various 2D transformation tasks, e.g. with Path objects.
//
class AffineTransform {
public:
	//
	// Creates an identity transform.
	//
	AffineTransform() throw();
	//
	// Creates a copy of another transform.
	//
	AffineTransform (const AffineTransform& other) throw();
	//
	// Creates a transform from a set of raw matrix values.
	// The resulting matrix is:
	//   (mat00 mat01 mat02)
	//   (mat10 mat11 mat12)
	//   (  0     0     1  )
	//
	AffineTransform (const float mat00, const float mat01, const float mat02,
	const float mat10, const float mat11, const float mat12) throw();
	//
	// Copies from another AffineTransform object
	//
	const AffineTransform& operator= (const AffineTransform& other) throw();
	//
	// Compares two transforms.
	//
	int    operator == (const AffineTransform& other) const throw();
	//
	// Compares two transforms.
	//
	int    operator!= (const AffineTransform& other) const throw();
	//
	// A ready-to-use identity transform, which you can use to append other transformations to.
	// e.g. @code
	//   AffineTransform myTransform = AffineTransform::identity.rotated (.5f).scaled (2.0f);
	// @endcode
	//
	static const AffineTransform identity;
	//
	// Transforms a 2D co-ordinate using this matrix.
	//
	void   TransformPoint(FPoint * p) const;
	//
	// Transforms a 2D co-ordinate using this matrix.
	//
	void   TransformPoint(double& x, double& y) const;
	//
	// Returns a new transform which is the same as this one followed by a translation.
	//
	const  AffineTransform Translated(FPoint delta) const;
	//
	// Returns a new transform which is a translation.
	//
	static const AffineTransform Translation(FPoint delta);
	//
	// Returns a transform which is the same as this one followed by a rotation.
	// The rotation is specified by a number of radians to rotate clockwise, centred around he origin (0, 0).
	//
	const  AffineTransform Rotated(const float angleInRadians) const;
	//
	// Returns a transform which is the same as this one followed by a rotation about a given point.
	// The rotation is specified by a number of radians to rotate clockwise, centred around
	// the co-ordinates passed in.
	//
	const AffineTransform Rotated(float angleInRadians, FPoint pivot) const;
	//
	// Returns a new transform which is a rotation about (0, 0).
	//
	static const AffineTransform Rotation(float angleInRadians);
	//
	// Returns a new transform which is a rotation about a given point.
	//
	static const AffineTransform Rotation(float angleInRadians, FPoint pivot);
	//
	// Returns a transform which is the same as this one followed by a re-scaling.
	// The scaling is centred around the origin (0, 0).
	//
	const  AffineTransform Scaled(FPoint factor) const;
	//
	// Returns a new transform which is a re-scale about the origin.
	//
	static const AffineTransform scale (const float factorX, const float factorY) throw();
	//
	// Returns a transform which is the same as this one followed by a shear.
	// The shear is centred around the origin (0, 0).
	//
	const AffineTransform sheared (const float shearX, const float shearY) const throw();
	//
	// Returns a matrix which is the inverse operation of this one.
	// Some matrices don't have an inverse - in this case, the method will just return an identity transform.
	//
	const AffineTransform inverted() const throw();
	//==============================================================================
	//
	// Returns the result of concatenating another transformation after this one.
	//
	const AffineTransform followedBy (const AffineTransform& other) const throw();
	//
	// Returns true if this transform has no effect on points.
	//
	int    isIdentity() const throw();
	//
	// Returns true if this transform maps to a singularity - i.e. if it has no inverse.
	//
	int    isSingularity() const throw();
	//
	// juce_UseDebuggingNewOperator
	//
	// The transform matrix is:
	//     (mat00 mat01 mat02)
	//     (mat10 mat11 mat12)
	//     (  0     0     1  )
	//
	float  mat00, mat01, mat02;
	float  mat10, mat11, mat12;
private:
	//==============================================================================
	const  AffineTransform followedBy (const float mat00, const float mat01, const float mat02,
		const float mat10, const float mat11, const float mat12) const throw();
};
//
//
//
class FLine_ {
public:
	//
	// Creates a line, using (0, 0) as its start and end points.
	//
	FLine();
	FLine(const FLine& other);
	FLine(FPoint start, FPoint end);
	const FLine & operator= (const FLine & other);
	~FLine();
	FPoint GetStart() const { return Start; }
	FPoint GetEnd() const { return End; }
	void   SetStart(FPoint);
	void   SetEnd(FPoint);
	//
	// Applies an affine transform to the line's start and end points.
	//
	void   ApplyTransform(const AffineTransform& transform);
	//
	// Returns the length of the line.
	//
	float  GetLength() const;
	//
	// Returns true if the line's start and end x co-ordinates are the same.
	//
	int    IsVertical() const;
	//
	// Returns true if the line's start and end y co-ordinates are the same.
	//
	int    IsHorizontal() const;
	//
	// Returns the line's angle.
	// This value is the number of radians clockwise from the 3 o'clock direction,
	// where the line's start point is considered to be at the centre.
	//
	float  GetAngle() const;
	//
	// Compares two lines.
	//
	int    IsEqual(const FLine &) const;
	//
	// Finds the intersection between two lines.
	// @param line             the other line
	// @param intersectionX    the x co-ordinate of the point where the lines meet (or
	// where they would meet if they were infinitely long)
	// the intersection (if the lines intersect). If the lines
	// are parallel, this will just be set to the position
	// of one of the line's endpoints.
	// @param intersectionY    the y co-ordinate of the point where the lines meet
	// @returns    true if the line segments intersect; false if they dont. Even if they
	// don't intersect, the intersection co-ordinates returned will still be valid
	//
	int    Intersects(const FLine & line, FPoint * pIntersection) const;
	//
	// Returns the location of the point which is a given distance along this line.
	// @param distanceFromStart    the distance to move along the line from its
	//   start point. This value can be negative or longer than the line itself
	//
	FPoint GetPointAlongLine(float distanceFromStart) const;
	//
	// Returns a point which is a certain distance along and to the side of this line.
	// This effectively moves a given distance along the line, then another distance
	// perpendicularly to this, and returns the resulting position.
	// @param distanceFromStart    the distance to move along the line from its
	//   start point. This value can be negative or longer
	//   than the line itself
	// @param perpendicularDistance    how far to move sideways from the line. If you're
	//   looking along the line from its start towards its end, then a positive value here
	//   will move to the right, negative value move to the left.
	//
	FPoint GetPointAlongLine(const float distanceFromStart, const float perpendicularDistance) const;
	//
	// Returns the location of the point which is a given distance along this line
	// proportional to the line's length.
	// @param proportionOfLength   the distance to move along the line from its
	//   start point, in multiples of the line's length.
	//   So a value of 0.0 will return the line's start point
	//   and a value of 1.0 will return its end point. (This value
	//   can be negative or greater than 1.0).
	//
	FPoint GetPointAlongLineProportionally(const float proportionOfLength) const;
	//
	// Returns the smallest distance between this line segment and a given point.
	// So if the point is close to the line, this will return the perpendicular
	// distance from the line; if the point is a long way beyond one of the line's
	// end-point's, it'll return the straight-line distance to the nearest end-point.
	// @param x    x position of the point to test
	// @param y    y position of the point to test
	// @returns the point's distance from the line
	//
	float  GetDistanceFromLine(FPoint p) const;
	//
	// Finds the point on this line which is nearest to a given point, and
	// returns its position as a proportional position along the line.
	// @param x    x position of the point to test
	// @param y    y position of the point to test
	// @returns    a value 0 to 1.0 which is the distance along this line from the
	//   line's start to the point which is nearest to the point passed-in. To
	//   turn this number into a position, use getPointAlongLineProportionally().
	//
	float  FindNearestPointTo(FPoint p) const;
	//
	// Returns true if the given point lies above this line.
	// The return value is true if the point's y coordinate is less than the y
	// coordinate of this line at the given x (assuming the line extends infinitely in both directions).
	//
	int    IsPointAbove(FPoint p) const;
	//
	// Returns a shortened copy of this line.
	// This will chop off part of the start of this line by a certain amount, (leaving the
	// end-point the same), and return the new line.
	//
	const FLine WithShortenedStart(float distanceToShortenBy) const;
	//
	// Returns a shortened copy of this line. This will chop off part of the end of this
	// line by a certain amount, (leaving the start-point the same), and return the new line.
	//
	const FLine WithShortenedEnd(float distanceToShortenBy) const;
	//
	// Cuts off parts of this line to keep the parts that are either inside or outside a path.
	//
	// Note that this isn't smart enough to cope with situations where the
	// line would need to be cut into multiple pieces to correctly clip against
	// a re-entrant shape.
	// @param path                     the path to clip against
	// @param keepSectionOutsidePath   if true, it's the section outside the path
	//   that will be kept; if false its the section inside the path
	// Returns:
	//   true if the line was changed.
	//
	int    ClipToPath(const GeomPath & path, int keepSectionOutsidePath);
	//==============================================================================
	//juce_UseDebuggingNewOperator
private:
	FPoint Start;
	FPoint End;
};
//
//
//
class GeomPath : private FloatArray {
public:
	GeomPath();
	GeomPath(const GeomPath &);
	~GeomPath();
	const  GeomPath & operator = (const GeomPath &);
	int    Copy(const GeomPath & s);
	int    IsEmpty() const;
	void   GetBounds(FPoint * pLU, FPoint * pSize) const;
	//
	// Returns the smallest rectangle that contains all points within the path
	// after it's been transformed with the given tranasform matrix.
	//
	void   GetBoundsTransformed(const AffineTransform & transform, FPoint * pLU, FPoint * pSize) const;
	//
	// Checks whether a point lies within the path.
	// This is only relevent for closed paths (see closeSubPath()), and
	// may produce false results if used on a path which has open sub-paths.
	// The path's winding rule is taken into account by this method.
	//
	int    Contains(FPoint) const;
	//
	// Checks whether a line crosses the path.
	// This will return positive if the line crosses any of the paths constituent
	// lines or curves. It doesn't take into account whether the line is inside
	// or outside the path, or whether the path is open or closed.
	//
	int    IntersectsLine(FPoint a, FPoint b);
	//
	// Removes all lines and curves, resetting the path completely.
	//
    void   Clear();
	//
	// Begins a new subpath with a given starting position.
	// This will move the path's current position to the co-ordinates passed in and
	// make it ready to draw lines or curves starting from this position.
	// After adding whatever lines and curves are needed, you can either
	// close the current sub-path using closeSubPath() or call StartNewSubPath()
	// to move to a new sub-path, leaving the old one open-ended.
	//
    void   StartNewSubPath(FPoint p);
	//
	// Closes a the current sub-path with a line back to its start-point.
	// When creating a closed shape such as a triangle, don't use 3 lineTo()
	// calls - instead use two lineTo() calls, followed by a closeSubPath()
	// to join the final point back to the start.
	// This ensures that closes shapes are recognised as such, and this is
	// important for tasks like drawing strokes, which needs to know whether to
	// draw end-caps or not.
	//
    void   CloseSubPath();
	//
	// Adds a line from the shape's last position to a new end-point.
	// This will connect the end-point of the last line or curve that was added
	// to a new point, using a straight line.
	// See the class description for an example of how to add lines and curves to a path.
	//
	void   LineTo(FPoint p);
	//
	// Adds a quadratic bezier curve from the shape's last position to a new position.
	// This will connect the end-point of the last line or curve that was added
	// to a new point, using a quadratic spline with one control-point.
	// See the class description for an example of how to add lines and curves to a path.
	//
	void   QuadraticTo(FPoint control, FPoint end);
	//
	// Adds a cubic bezier curve from the shape's last position to a new position.
	// This will connect the end-point of the last line or curve that was added
	// to a new point, using a cubic spline with two control-points.
	// See the class description for an example of how to add lines and curves to a path.
	//
	void   CubicTo(FPoint control1, FPoint control2, FPoint end);
	//
	// Returns the last point that was added to the path by one of the drawing methods.
	//
	const FPoint GetCurrentPosition() const;
	//
	// Adds a rectangle to the path.
	// The rectangle is added as a new sub-path. (Any currently open paths will be left open).
	//
	void   AddRectangle(FPoint lu, FPoint size);
	//
	// Adds a rectangle with rounded corners to the path.
	// The rectangle is added as a new sub-path. (Any currently open paths will be left open).
	//
	void   AddRoundedRectangle(FPoint lu, FPoint size, float cornerSize);
	//
	// Adds a rectangle with rounded corners to the path.
	// The rectangle is added as a new sub-path. (Any currently open paths will be left open).
	//
	void   AddRoundedRectangle(FPoint lu, FPoint size, FPoint corner);
	//
	// Adds a triangle to the path.
	// The triangle is added as a new closed sub-path. (Any currently open paths will be left open).
	// Note that whether the vertices are specified in clockwise or anticlockwise
	// order will affect how the triangle is filled when it overlaps other
	// shapes (the winding order setting will affect this of course).
	//
	void   AddTriangle(FPoint p1, FPoint p2, FPoint p3);
	//
	// Adds a quadrilateral to the path.
	// The quad is added as a new closed sub-path. (Any currently open paths will be left open).
	// Note that whether the vertices are specified in clockwise or anticlockwise
	// order will affect how the quad is filled when it overlaps other
	// shapes (the winding order setting will affect this of course).
	//
	void   AddQuadrilateral(FPoint p1, FPoint p2, FPoint p3, FPoint p4);
	//
	// Adds an ellipse to the path.
	// The shape is added as a new sub-path. (Any currently open paths will be left open).
	//
	void   AddEllipse(FPoint lu, FPoint size);
	//
	// Adds an elliptical arc to the current path.
	// Note that when specifying the start and end angles, the curve will be drawn either clockwise
	// or anti-clockwise according to whether the end angle is greater than the start. This means
	// that sometimes you may need to use values greater than 2*Pi for the end angle.
	// @param edgeLU       the left-upper edge of the rectangle in which the elliptical outline fits
	// @param size         the size of the rectangle in which the elliptical outline fits
	// @param fromRadians  the angle (clockwise) in radians at which to start the arc segment (where 0 is the
	//   top-centre of the ellipse)
	// @param toRadians    the angle (clockwise) in radians at which to end the arc segment (where 0 is the
	//   top-centre of the ellipse). This angle can be greater than 2*Pi, so for example to
	//   draw a curve clockwise from the 9 o'clock position to the 3 o'clock position via
	//   12 o'clock, you'd use 1.5*Pi and 2.5*Pi as the start and finish points.
	// @param startAsNewSubPath    if true, the arc will begin a new subpath from its starting point; if false,
	//   it will be added to the current sub-path, continuing from the current postition
	//
	void   AddArc(FPoint edgeLU, FPoint size, float fromRadians, float toRadians, int startAsNewSubPath = 0);
	//
	// Adds an arc which is centred at a given point, and can have a rotation specified.
	// Note that when specifying the start and end angles, the curve will be drawn either clockwise
	// or anti-clockwise according to whether the end angle is greater than the start. This means
	// that sometimes you may need to use values greater than 2*Pi for the end angle.
	// @param centre       the centre of the ellipse
	// @param radius       the {horizontal, vertical} radius of the ellipse
	// @param rotationOfEllipse    an angle by which the whole ellipse should be rotated about its centre, in radians (clockwise)
	// @param fromRadians  the angle (clockwise) in radians at which to start the arc segment (where 0 is the
	//   top-centre of the ellipse)
	// @param toRadians    the angle (clockwise) in radians at which to end the arc segment (where 0 is the
	//   top-centre of the ellipse). This angle can be greater than 2*Pi, so for example to
	//   draw a curve clockwise from the 9 o'clock position to the 3 o'clock position via
	//   12 o'clock, you'd use 1.5*Pi and 2.5*Pi as the start and finish points.
	// @param startAsNewSubPath    if true, the arc will begin a new subpath from its starting point; if false,
	//   it will be added to the current sub-path, continuing from the current postition
	//
	void   AddCentredArc(FPoint centre, FPoint radius, float rotationOfEllipse, float fromRadians,
		float toRadians, int startAsNewSubPath = 0);
	//
	// Adds a "pie-chart" shape to the path.
	// The shape is added as a new sub-path. (Any currently open paths will be left open).
	// Note that when specifying the start and end angles, the curve will be drawn either clockwise
	// or anti-clockwise according to whether the end angle is greater than the start. This means
	// that sometimes you may need to use values greater than 2*Pi for the end angle.
	// @param edgeLU       the left-upper edge of the rectangle in which the elliptical outline fits
	// @param size         the size of the rectangle in which the elliptical outline fits
	// @param fromRadians  the angle (clockwise) in radians at which to start the arc segment (where 0 is the
	//   top-centre of the ellipse)
	// @param toRadians    the angle (clockwise) in radians at which to end the arc segment (where 0 is the
	//   top-centre of the ellipse)
	// @param innerCircleProportionalSize  if this is > 0, then the pie will be drawn as a curved band around a hollow
	//   ellipse at its centre, where this value indicates the inner ellipse's size with
	//   respect to the outer one.
	//
	void   AddPieSegment(FPoint edgeLU, FPoint size, const float fromRadians, const float toRadians,
		const float innerCircleProportionalSize);
	//
	// Adds a line with a specified thickness.
	// The line is added as a new closed sub-path. (Any currently open paths will be left open).
	//
	void   AddLineSegment(FPoint start, FPoint end, float lineThickness);
	//
	// Adds a line with an arrowhead on the end.
	// The arrow is added as a new closed sub-path. (Any currently open paths will be left open).
	//
	void   AddArrow(FPoint start, FPoint end, float lineThickness, FPoint arrowheadSize);
	//
	// Adds a star shape to the path.
	//
	void   AddStar(FPoint centre, int numberOfPoints, float innerRadius, float outerRadius, float startAngle = 0.0f);
	//
	// Adds a speech-bubble shape to the path.
	// @param bodyLU           the left-upper of the main body area of the bubble
	// @param bodySize         the size of the main body area of the bubble
	// @param cornerSize       the amount by which to round off the corners of the main body rectangle
	// @param arrowTip         the position that the tip of the arrow should connect to
	// @param whichSide        the side to connect the arrow to: 0 = top, 1 = left, 2 = bottom, 3 = right
	// @param arrowPositionAlongEdgeProportional   how far along the edge of the main rectangle the
	//    arrow's base should be - this is a proportional distance between 0 and 1.0
	// @param arrowWidth       how wide the base of the arrow should be where it joins the main rectangle
	//
	void AddBubble(FPoint bodyLU, FPoint bodySize, float cornerSize, FPoint arrowTip,
		int whichSide, float arrowPositionAlongEdgeProportional, float arrowWidth);
	//
	// Adds another path to this one.
	// The new path is added as a new sub-path. (Any currently open paths in this
	// path will be left open).
	// @param pathToAppend     the path to add
	//
	void   AddPath(const GeomPath & pathToAppend);
	//
	// Adds another path to this one, transforming it on the way in.
	// The new path is added as a new sub-path, its points being transformed by the given
	// matrix before being added.
	//
	// @param pathToAppend     the path to add
	// @param transformToApply an optional transform to apply to the incoming vertices
	//
	void   AddPath(const GeomPath & pathToAppend, const AffineTransform & transformToApply);
	//
	// Swaps the contents of this path with another one.
	// The internal data of the two paths is swapped over, so this is much faster than
	// copying it to a temp variable and back.
	//
	void   SwapWithPath(GeomPath & other);
	//
	// Applies a 2D transform to all the vertices in the path.
	//
	void   ApplyTransform(const AffineTransform & transform);
	//
	// Rescales this path to make it fit neatly into a given space.
	// This is effectively a quick way of calling
	// ApplyTransform (getTransformToScaleToFit (x, y, w, h, preserveProportions))
	// @param lu                   the left-upper position of the rectangle to fit the path inside
	// @param size                 the size of the rectangle to fit the path inside
	// @param preserveProportions  if true, it will fit the path into the space without altering its
	//   horizontal/vertical scale ratio; if false, it will distort the
	//   path to fill the specified ratio both horizontally and vertically
	//
	void   ScaleToFit(FPoint lu, FPoint size, int preserveProportions);
	//
	// Returns a transform that can be used to rescale the path to fit into a given space.
	// @param x                    the x position of the rectangle to fit the path inside
	// @param y                    the y position of the rectangle to fit the path inside
	// @param width                the width of the rectangle to fit the path inside
	// @param height               the height of the rectangle to fit the path inside
	// @param preserveProportions  if true, it will fit the path into the space without altering its
	//   horizontal/vertical scale ratio; if false, it will distort the
	//   path to fill the specified ratio both horizontally and vertically
	// @param justificationType    if the proportions are preseved, the resultant path may be smaller
	//   than the available rectangle, so this describes how it should be
	//   positioned within the space.
	// Returns:
	//   an appropriate transformation
	//
	const AffineTransform GetTransformToScaleToFit(FPoint lu, FPoint size,
		int preserveProportions, const Justification & justificationType = Justification::centred) const;
	//
	// Creates a version of this path where all sharp corners have been replaced by curves.
	// Wherever two lines meet at an angle, this will replace the corner with a curve of the given radius.
	//
    const GeomPath CreatePathWithRoundedCorners(float cornerRadius) const;
	//
	// Changes the winding-rule to be used when filling the path.
	// If set to true (which is the default), then the path uses a non-zero-winding rule
	// to determine which points are inside the path. If set to false, it uses an
	// alternate-winding rule.
	// The winding-rule comes into play when areas of the shape overlap other
	// areas, and determines whether the overlapping regions are considered to be inside or outside.
	// Changing this value just sets a flag - it doesn't affect the contents of the path.
	//
	void   SetUsingNonZeroWinding(int isNonZeroWinding);
	//
	// Returns the flag that indicates whether the path should use a non-zero winding rule.
	// The default for a new path is true.
	//
	int    isUsingNonZeroWinding() const { return useNonZeroWinding; }
	//
	// Iterates the lines and curves that a path contains.
	//
	class Iterator {
	public:
		Iterator(const GeomPath& path);
		//
		// Moves onto the next element in the path.
		// If this returns false, there are no more elements. If it returns true,
		// the elementType variable will be set to the type of the current element,
		// and some of the x and y variables will be filled in with values.
		//
		int    Next();
        enum PathElementType {
			StartNewSubPath, // For this type, p1 will be set to indicate the first point in the subpath.
			lineTo,          // For this type, p1 indicate the end point of the line.
			quadraticTo,     // For this type, p1, p2 indicate the control point and endpoint of a quadratic curve.
			cubicTo,         // For this type, p1, p2, p3 indicate the two control points and the endpoint of a cubic curve
			closePath        // Indicates that the sub-path is being closed. None of the x or y values are valid in this case.
		};
		PathElementType elementType;
		FPoint p1, p2, p3;
	private:
		const  GeomPath & path;
		uint   Index;
    };
	//
	// Loads a stored path from a data stream.
	// The data in the stream must have been written using writePathToStream().
	// Note that this will append the stored path to whatever is currently in
	// this path, so you might need to call clear() beforehand.
	//
	int    Read(SBuffer &);
	//
	// Stores the path by writing it out to a stream.
	// After writing out a path, you can reload it using loadPathFromStream().
	//
	int    Write(SBuffer &) const;
	//
	// Creates a string containing a textual representation of this path.
	//
	int    ToString(SString &) const;
	//
	// Restores this path from a string that was created with the toString() method.
	//
	int    FromString(const char * pStr);
	//
	//juce_UseDebuggingNewOperator
private:
	friend class PathFlatteningIterator;
	friend class GeomPath::Iterator;
	//int    numElements;
	FPoint PathMin;
	FPoint PathMax;
	int    useNonZeroWinding;
};
//
// Flattens a Path object into a series of straight-line sections.
//
// Use one of these to iterate through a Path object, and it will convert
// all the curves into line sections so it's easy to render or perform
// geometric operations on.
//
class PathFlatteningIterator {
public:
	//
	// Creates a PathFlatteningIterator.
	// After creation, use the next() method to initialise the fields in the
	// object with the first line's position.
	// @param path         the path to iterate along
	// @param transform    a transform to apply to each point in the path being iterated
	// @param tolerence    the amount by which the curves are allowed to deviate from the
	//   lines into which they are being broken down - a higher tolerence
	//   is a bit faster, but less smooth.
	//
	PathFlatteningIterator(const GeomPath & path,
		const AffineTransform & transform = AffineTransform::identity, float tolerence = 9.0f);
	//
	// Fetches the next line segment from the path.
	// This will update the member variables x1, y1, x2, y2, subPathIndex and closesSubPath
	// so that they describe the new line segment.
	// @returns false when there are no more lines to fetch.
	//
	int    Next();
	FPoint p1; // The start of the current line segment
	FPoint p2; // The end of the current line segment
	//
	// Indicates whether the current line segment is closing a sub-path.
	// If the current line is the one that connects the end of a sub-path
	// back to the start again, this will be true.
	//
	int    ClosesSubPath;
	//
	// The index of the current line within the current sub-path.
	// E.g. you can use this to see whether the line is the first one in the
	// subpath by seeing if it's 0.
	//
	int    SubPathIndex;

	//juce_UseDebuggingNewOperator
private:
	PathFlatteningIterator(const PathFlatteningIterator&);
	const PathFlatteningIterator& operator= (const PathFlatteningIterator&);

	const  GeomPath & path;
	const  AffineTransform transform;
	FloatArray Points;
	float  tolerence;
	FPoint SubPathClose;
	int    isIdentityTransform;
	FloatStack Stack;
	uint   index;
};

//==============================================================================
PathFlatteningIterator::PathFlatteningIterator(const GeomPath & path_, const AffineTransform& transform_, float tolerence_) :
	p2(0.f), ClosesSubPath(0), SubPathIndex(-1), path(path_), transform(transform_),
	Points(path_), tolerence(tolerence_ * tolerence_)
{
	SubPathClose.SetZero();
	index = 0;
	isIdentityTransform = transform.isIdentity();
}

int PathFlatteningIterator::Next()
{
	p1 = p2;

	FPoint p3(0.f);
	FPoint p4(0.f);
	float type;
	for(;;) {
		if(Stack.getPointer() == 0) {
			if(index >= path.getCount())
				return 0;
			else {
				type = Points.get(index++);
				if(type != closePathMarker) {
					p2 = Points.getPoint(index);
					index += 2;
					if(!isIdentityTransform)
						transform.TransformPoint(&p2);
					if(type == quadMarker) {
						p3 = Points.getPoint(index);
						index += 2;
						if(!isIdentityTransform)
							transform.TransformPoint(&p3);
					}
					else if(type == cubicMarker) {
						p3 = Points.getPoint(index);
						index += 2;
						p4 = Points.getPoint(index);
						index += 2;
						if(!isIdentityTransform) {
							transform.TransformPoint(&p3);
							transform.TransformPoint(&p4);
						}
					}
				}
			}
		}
		else {
			Stack.pop(type);
			if(type != closePathMarker) {
				Stack.popPoint(p2);
				if(type == quadMarker)
					Stack.popPoint(p3);
				else if(type == cubicMarker) {
					Stack.popPoint(p3);
					Stack.popPoint(p4);
				}
			}
		}
		if(type == lineMarker) {
			++SubPathIndex;
			ClosesSubPath = (Stack.getPointer() == 0) && (index < path.getCount()) &&
				(Points.get(index) == closePathMarker) && p2 == SubPathClose;
			return true;
		}
		else if(type == quadMarker) {
			FPoint d1 = p1 - p2;
			FPoint d2 = p2 - p3;

			FPoint m1 = (p1 + p2) * 0.5f;
			FPoint m2 = (p2 + p3) * 0.5f;
			FPoint m3 = (m1 + m2) * 0.5f;
			if((d1.Sq() + d2.Sq()) > tolerence) {
				Stack.pushPoint(p3);
				Stack.pushPoint(m2);
				Stack.push(quadMarker);

				Stack.pushPoint(m3);
				Stack.pushPoint(m1);
				Stack.push(quadMarker);
			}
			else {
				Stack.pushPoint(p3);
				Stack.push(lineMarker);

				Stack.pushPoint(m3);
				Stack.push(lineMarker);
			}
		}
		else if(type == cubicMarker) {
			FPoint d1 = p1 - p2;
			FPoint d2 = p2 - p3;
			FPoint d3 = p3 - p4;

			FPoint m1 = (p1 + p2) * 0.5f;
			FPoint m2 = (p2 + p3) * 0.5f;
			FPoint m3 = (p3 + p4) * 0.5f;
			FPoint m4 = (m1 + m2) * 0.5f;
			FPoint m5 = (m2 + m3) * 0.5f;
			if((d1.Sq() + d2.Sq() + d3.Sq()) > tolerence) {
				Stack.pushPoint(p4);
				Stack.pushPoint(m3);
				Stack.pushPoint(m5);
				Stack.push(cubicMarker);

				Stack.pushPoint((m4 + m5) * 0.5f);
				Stack.pushPoint(m4);
				Stack.pushPoint(m1);
				Stack.push(cubicMarker);
			}
			else {
				Stack.pushPoint(p4);
				Stack.push(lineMarker);

				Stack.pushPoint(m5);
				Stack.push(lineMarker);

				Stack.pushPoint(m4);
				Stack.push(lineMarker);
			}
		}
		else if(type == closePathMarker) {
			if(!(p2 == SubPathClose)) {
				p1 = p2;
				p2 = SubPathClose;
				ClosesSubPath = 1;
				return true;
			}
		}
		else {
			SubPathIndex = -1;
			p1 = p2;
			SubPathClose.Set(p1.X, p2.Y);
		}
	}
}
//
// @ModuleDef(GeomPath)
//
GeomPath::GeomPath() : FloatArray()
{
	PathMin.Set(0);
	PathMax.Set(0);
	useNonZeroWinding = 1;
}

GeomPath::~GeomPath()
{
}

GeomPath::GeomPath(const GeomPath & s) : FloatArray()
{
	GeomPath::Copy(s);
}

const GeomPath & GeomPath::operator=(const GeomPath & s)
{
    if(this != &s)
		GeomPath::Copy(s);
    return *this;
}

int GeomPath::Copy(const GeomPath & s)
{
	PathMin = s.PathMin;
	PathMax = s.PathMax;
	useNonZeroWinding = s.useNonZeroWinding;
	return FloatArray::copy(s);
}

void GeomPath::Clear()
{
	FloatArray::clear();
	PathMin.Set(0);
	PathMax.Set(0);
}

/*
void GeomPath::SwapWithPath(GeomPath & other)
{
    swapVariables <int>(this->numAllocated, other.numAllocated);
    swapVariables <float*>(this->elements, other.elements);
    swapVariables <int>(this->numElements, other.numElements);
    swapVariables <float>(this->pathXMin, other.pathXMin);
    swapVariables <float>(this->pathXMax, other.pathXMax);
    swapVariables <float>(this->pathYMin, other.pathYMin);
    swapVariables <float>(this->pathYMax, other.pathYMax);
    swapVariables <bool>(this->useNonZeroWinding, other.useNonZeroWinding);
}
*/

void GeomPath::SetUsingNonZeroWinding(int isNonZero)
{
	useNonZeroWinding = isNonZero;
}

void GeomPath::ScaleToFit(FPoint lu, FPoint size, int preserveProportions)
{
	ApplyTransform(GetTransformToScaleToFit(lu, size, preserveProportions));
}

//==============================================================================
int GeomPath::IsEmpty() const
{
	for(uint i = 0; i < getCount();) {
        const float type = get(i++);
        if(type == moveMarker)
            i += 2;
        else if(oneof3(type, lineMarker, quadMarker, cubicMarker))
            return 0;
    }
    return 1;
}

void GeomPath::GetBounds(FPoint * pLU, FPoint * pSize) const
{
	if(pLU)
		*pLU = PathMin;
	if(pSize)
		*pSize = PathMax - PathMin;
}

void GeomPath::GetBoundsTransformed(const AffineTransform & transform, FPoint * pLU, FPoint * pSize) const
{
	FPoint p1 = PathMin;
    transform.TransformPoint(&p1);

	FPoint p2(PathMax.X, PathMin.Y);
    transform.TransformPoint(&p2);

	FPoint p3(PathMin.X, PathMax.Y);
    transform.TransformPoint(&p3);

	FPoint p4(PathMax.X, PathMax.Y);
    transform.TransformPoint(&p4);

	pLU->Set(fmin4(p1.X, p2.X, p3.X, p4.X), fmin4(p1.Y, p2.Y, p3.Y, p4.Y));
	*pSize = FPoint(fmax4(p1.X, p2.X, p3.X, p4.X), fmax4(p1.Y, p2.Y, p3.Y, p4.Y)) - *pLU;
}

//==============================================================================
void GeomPath::StartNewSubPath(FPoint p)
{
    if(getCount() == 0) {
		PathMin = PathMax = p;
    }
    else {
		PathMin = fmin(PathMin, p);
		PathMax = fmax(PathMax, p);
    }
	add(moveMarker);
	add(p);
}

void GeomPath::LineTo(FPoint p) throw()
{
	if(getCount() == 0)
		StartNewSubPath(FPoint(0, 0));
	add(lineMarker);
	add(p);
	PathMin = fmin(PathMin, p);
	PathMax = fmax(PathMax, p);
}

void GeomPath::QuadraticTo(FPoint p1, FPoint p2)
{
	if(getCount() == 0)
		StartNewSubPath(FPoint(0, 0));
	add(quadMarker);
	add(p1);
	add(p2);
	PathMin = fmin(fmin(PathMin, p1), p2);
	PathMax = fmax(fmax(PathMax, p1), p2);
}

void GeomPath::CubicTo(FPoint p1, FPoint p2, FPoint p3)
{
	if(getCount() == 0)
		StartNewSubPath(FPoint(0, 0));
	add(cubicMarker);
	add(p1);
	add(p2);
	add(p3);
	PathMin = fmin(fmin(fmin(PathMin, p1), p2), p3);
	PathMax = fmax(fmax(fmax(PathMax, p1), p2), p3);
}

void GeomPath::CloseSubPath()
{
	const uint c = getCount();
	if(c && get(c - 1) != closeSubPathMarker)
		add(closeSubPathMarker);
}

const FPoint GeomPath::GetCurrentPosition() const
{
	const  uint c = getCount();
	uint   i = c;
	if(i && get(--i) == closeSubPathMarker)
		do {
            if(get(i) == moveMarker) {
                i += 2;
				assert(i < c);
                break;
            }
        } while(--i);
    return (i > 0) ? FPoint(get(i-1), get(i)) : ZEROFPOINT;
}

void GeomPath::AddRectangle(FPoint p, FPoint size)
{
	StartNewSubPath(p.AddY(size.Y));
	LineTo(p);
	LineTo(p.AddX(size.X));
	LineTo(p + size);
	CloseSubPath();
}

void GeomPath::AddRoundedRectangle(FPoint p, FPoint size, FPoint corner) throw()
{
	corner = fmin(corner, size * 0.5f);
	FPoint corner45 = corner * 0.45f;
	FPoint p2 = p + size;

	StartNewSubPath(p.AddX(corner.X)); // Начинаем в верхнем левом углу со смещением вправо на corner.X

	FPoint tmp(p2.X, p.Y);  // Верхний правый угол
	LineTo(tmp.AddX(-corner.X)); // Гориз линия до правого угла со смещением -corner.X
	CubicTo(tmp.AddX(-corner45.X), tmp.AddY(corner45.Y), tmp.AddY(corner.Y));

	LineTo(p2.AddY(-corner.Y));
	CubicTo(p2.AddY(-corner45.Y), p2.AddX(-corner45.X), p2.AddX(-corner.X));

	tmp.Set(p.X, p2.Y); // Нижний левый угол
	LineTo(tmp.AddX(corner.X));
	CubicTo(tmp.AddX(corner45.X), tmp.AddY(-corner45.Y), tmp.AddY(-corner.Y));

	LineTo(p.AddY(corner.Y));
	CubicTo(p.AddY(corner45.Y), p.AddX(corner45.X), p.AddX(corner.X));
	CloseSubPath();
}

void GeomPath::AddRoundedRectangle(FPoint p, FPoint size, float corner)
{
	AddRoundedRectangle(p, size, FPoint(corner, corner));
}

void GeomPath::AddTriangle(FPoint p1, FPoint p2, FPoint p3)
{
	StartNewSubPath(p1);
	LineTo(p2);
	LineTo(p3);
	CloseSubPath();
}

void GeomPath::AddQuadrilateral(FPoint p1, FPoint p2, FPoint p3, FPoint p4)
{
	StartNewSubPath(p1);
	LineTo(p2);
	LineTo(p3);
	LineTo(p4);
	CloseSubPath();
}

void GeomPath::AddEllipse(FPoint p, FPoint size)
{
	FPoint hsz = size * 0.5f;
	FPoint hsz55 = hsz * 0.55f;
	FPoint cp = p + hsz;

	StartNewSubPath(cp.AddY(-hsz.Y));
	FPoint w55_h(hsz55.X, -hsz.Y);
	FPoint wh55(hsz.X, hsz55.Y);
	FPoint w_h55(hsz.X, -hsz55.Y);
	FPoint w55h(hsz55.X, hsz.Y);
	CubicTo(cp + w55_h, cp + w_h55, cp.AddX(hsz.X));
	CubicTo(cp + wh55,  cp + w55h,  cp.AddY(hsz.Y));
	CubicTo(cp - w55_h, cp - w_h55, cp.AddX(-hsz.X));
	CubicTo(cp - wh55,  cp - w55h,  cp.AddY(-hsz.Y));
	CloseSubPath();
}

void GeomPath::AddArc(FPoint edgeLU, FPoint edgeSize, float fromRadians, float toRadians, int startAsNewSubPath)
{
	FPoint radius = edgeSize / 2.0f;
	AddCentredArc(edgeLU + radius, radius, 0.0f, fromRadians, toRadians, startAsNewSubPath);
}

static const float ellipseAngularIncrement = 0.05f;

void GeomPath::AddCentredArc(FPoint centre, FPoint radius,
	float rotationOfEllipse, float fromRadians, float toRadians, int startAsNewSubPath)
{
	if(radius.X > 0.0f && radius.Y > 0.0f) {
		const AffineTransform rotation(AffineTransform::Rotation(rotationOfEllipse, centre));
		float angle = fromRadians;
		FPoint p;
		if(startAsNewSubPath) {
			p = trans01(centre, radius, angle);
			if(rotationOfEllipse != 0)
				rotation.TransformPoint(&p);
			StartNewSubPath(p);
		}
		if(fromRadians < toRadians) {
			if(startAsNewSubPath)
				angle += ellipseAngularIncrement;
			for(; angle < toRadians; angle += ellipseAngularIncrement) {
				p = trans01(centre, radius, angle);
				if(rotationOfEllipse != 0)
					rotation.TransformPoint(&p);
				LineTo(p);
			}
		}
		else {
			if(startAsNewSubPath)
				angle -= ellipseAngularIncrement;
			for(; angle > toRadians; angle -= ellipseAngularIncrement) {
				p = trans01(centre, radius, angle);
				if(rotationOfEllipse != 0)
					rotation.TransformPoint(&p);
				LineTo(p);
			}
		}
		p = trans01(centre, radius, toRadians);
		if(rotationOfEllipse != 0)
			rotation.TransformPoint(&p);
		LineTo(p);
	}
}

void GeomPath::AddPieSegment(FPoint p, FPoint size, float fromRadians, float toRadians, float innerCircleProportionalSize)
{
	FPoint sz = size * 0.5f;
	FPoint centre = p + sz;
	StartNewSubPath(trans01(centre, sz, fromRadians));
	AddArc(p, size, fromRadians, toRadians);
	if(fabs(fromRadians - toRadians) > SMathConst::Pi_f * 1.999f) {
		CloseSubPath();
		if(innerCircleProportionalSize > 0) {
			sz = sz * innerCircleProportionalSize;
			StartNewSubPath(trans01(centre, sz, toRadians));
			AddArc(centre - sz, sz * 2.0f, toRadians, fromRadians);
		}
	}
	else
		if(innerCircleProportionalSize > 0) {
			sz = sz * innerCircleProportionalSize;
			AddArc(centre - sz, sz * 2.0f, toRadians, fromRadians);
		}
		else
			LineTo(centre);
	CloseSubPath();
}

//==============================================================================
static FPoint PerpendicularOffset(FPoint p1, FPoint p2, FPoint offset)
{
	FPoint d = p2 - p1;
	const float len = d.Hypotf();
	if(len == 0)
		return p1;
	else {
		FPoint temp((d * offset).Sub(), (d * offset.Swap()).Add());
		return (p1 + temp / len);
	}
}

//==============================================================================
void GeomPath::AddLineSegment(FPoint start, FPoint end, float lineThickness)
{
	lineThickness *= 0.5f;
	FPoint offs(0, lineThickness);
	FPoint neg_offs(0, -lineThickness);
	StartNewSubPath(PerpendicularOffset(start, end, offs));
	LineTo(PerpendicularOffset(start, end, neg_offs));
	LineTo(PerpendicularOffset(end, start, offs));
	LineTo(PerpendicularOffset(end, start, neg_offs));
	CloseSubPath();
}

	// float arrowheadWidth, float arrowheadLength
void GeomPath::AddArrow(FPoint start, FPoint end, float lineThickness, FPoint arrowheadSize)
{
	lineThickness *= 0.5f;
	arrowheadSize.X *= 0.5f;
	arrowheadSize.Y = min(arrowheadSize.Y, 0.8f * (start - end).Hypotf());
	StartNewSubPath(PerpendicularOffset(start, end, FPoint(0, lineThickness)));
	LineTo(PerpendicularOffset(start, end, FPoint(0, -lineThickness)));
	LineTo(PerpendicularOffset(end, start, FPoint(arrowheadSize.Y, lineThickness)));
	LineTo(PerpendicularOffset(end, start, FPoint(arrowheadSize.Y, arrowheadSize.X)));
	LineTo(PerpendicularOffset(end, start, ZEROFPOINT));
	LineTo(PerpendicularOffset(end, start, FPoint(arrowheadSize.Y, -arrowheadSize.X)));
	LineTo(PerpendicularOffset(end, start, FPoint(arrowheadSize.Y, -lineThickness)));
	CloseSubPath();
}

void GeomPath::AddStar(FPoint centre, int numberOfPoints, float innerRadius, float outerRadius, float startAngle)
{
    assert(numberOfPoints > 1); // this would be silly.
    if(numberOfPoints > 1) {
        const float angleBetweenPoints = SMathConst::Pi_f * 2.0f / numberOfPoints;
        for(int i = 0; i < numberOfPoints; ++i) {
            float angle = startAngle + i * angleBetweenPoints;
			FPoint p = trans01(centre, outerRadius, angle);
            if(i == 0)
                StartNewSubPath(p);
            else
                LineTo(p);
			LineTo(trans01(centre, innerRadius, angle + angleBetweenPoints * 0.5f));
        }
        CloseSubPath();
    }
}

#if 0 // {
void Path::addBubble (float x, float y,
                      float w, float h,
                      float cs,
                      float tipX,
                      float tipY,
                      int whichSide,
                      float arrowPos,
                      float arrowWidth)
{
    if (w > 1.0f && h > 1.0f)
    {
        cs = jmin (cs, w * 0.5f, h * 0.5f);
        const float cs2 = 2.0f * cs;

        startNewSubPath (x + cs, y);

        if (whichSide == 0)
        {
            const float halfArrowW = jmin (arrowWidth, w - cs2) * 0.5f;
            const float arrowX1 = x + cs + jmax (0.0f, (w - cs2) * arrowPos - halfArrowW);
            lineTo (arrowX1, y);
            lineTo (tipX, tipY);
            lineTo (arrowX1 + halfArrowW * 2.0f, y);
        }

        lineTo (x + w - cs, y);

        if (cs > 0.0f)
            addArc (x + w - cs2, y, cs2, cs2, 0, SMathConst::Pi_f * 0.5f);

        if (whichSide == 3)
        {
            const float halfArrowH = jmin (arrowWidth, h - cs2) * 0.5f;
            const float arrowY1 = y + cs + jmax (0.0f, (h - cs2) * arrowPos - halfArrowH);
            lineTo (x + w, arrowY1);
            lineTo (tipX, tipY);
            lineTo (x + w, arrowY1 + halfArrowH * 2.0f);
        }

        lineTo (x + w, y + h - cs);

        if (cs > 0.0f)
            addArc (x + w - cs2, y + h - cs2, cs2, cs2, SMathConst::Pi_f * 0.5f, SMathConst::Pi_f);

        if (whichSide == 2)
        {
            const float halfArrowW = jmin (arrowWidth, w - cs2) * 0.5f;
            const float arrowX1 = x + cs + jmax (0.0f, (w - cs2) * arrowPos - halfArrowW);
            lineTo (arrowX1 + halfArrowW * 2.0f, y + h);
            lineTo (tipX, tipY);
            lineTo (arrowX1, y + h);
        }

        lineTo (x + cs, y + h);

        if (cs > 0.0f)
            addArc (x, y + h - cs2, cs2, cs2, SMathConst::Pi_f, SMathConst::Pi_f * 1.5f);

        if (whichSide == 1)
        {
            const float halfArrowH = jmin (arrowWidth, h - cs2) * 0.5f;
            const float arrowY1 = y + cs + jmax (0.0f, (h - cs2) * arrowPos - halfArrowH);
            lineTo (x, arrowY1 + halfArrowH * 2.0f);
            lineTo (tipX, tipY);
            lineTo (x, arrowY1);
        }

        lineTo (x, y + cs);

        if (cs > 0.0f)
            addArc (x, y, cs2, cs2, SMathConst::Pi_f * 1.5f, SMathConst::Pi_f * 2.0f - ellipseAngularIncrement);

        closeSubPath();
    }
}
#endif // } 0

void GeomPath::AddBubble(FPoint p, FPoint size, float cs, FPoint tip, int whichSide, float arrowPos, float arrowWidth)
{
	if(size.X > 1.0f && size.Y > 1.0f) {
		cs = fmin3(cs, size.X * 0.5f, size.Y * 0.5f);
		const float cs2 = 2.0f * cs;
		StartNewSubPath(p.AddX(cs));
		if(whichSide == 0) {
			const float w_cs2 = size.X - cs2;
			const float halfArrowW = fmin2(arrowWidth, w_cs2) * 0.5f;
			FPoint temp = p.AddX(cs + MAX(0.0f, w_cs2 * arrowPos - halfArrowW));
			LineTo(temp);
			LineTo(tip);
			LineTo(temp.AddX(halfArrowW * 2.0f));
		}
		LineTo(p.AddX(size.X - cs));
		if(cs > 0.0f)
			AddArc(p.AddX(size.X - cs2), FPoint(cs2), 0, SMathConst::Pi_f * 0.5f);
		if(whichSide == 3) {
			const float h_cs2 = size.Y - cs2;
			const float halfArrowH = fmin2(arrowWidth, h_cs2) * 0.5f;
			FPoint temp(p.X + size.X, p.Y + cs + fmax2(0.0f, h_cs2 * arrowPos - halfArrowH));
			LineTo(temp);
			LineTo(tip);
			LineTo(temp.AddY(halfArrowH * 2.0f));
		}
		LineTo((p + size).AddY(-cs));
		if(cs > 0.0f)
			AddArc(p + size - cs2, FPoint(cs2), SMathConst::Pi_f * 0.5f, SMathConst::Pi_f);
		if(whichSide == 2) {
			const float w_cs2 = size.X - cs2;
			const float halfArrowW = fmin2(arrowWidth, w_cs2) * 0.5f;
			FPoint temp = p + FPoint(cs + fmax2(0.0f, w_cs2 * arrowPos - halfArrowW), size.Y);
			LineTo(temp.AddX(halfArrowW * 2.0f));
			LineTo(tip);
			LineTo(temp);
		}
		LineTo(p + FPoint(cs, size.Y));
		if(cs > 0.0f)
			AddArc(p.AddY(size.Y - cs2), FPoint(cs2), SMathConst::Pi_f, SMathConst::Pi_f * 1.5f);
		if(whichSide == 1) {
			const float h_cs2 = size.Y - cs2;
			const float halfArrowH = fmin2(arrowWidth, h_cs2) * 0.5f;
			FPoint temp = p.AddY(cs + fmax2(0.0f, h_cs2 * arrowPos - halfArrowH));
			LineTo(temp.AddY(halfArrowH * 2.0f));
			LineTo(tip);
			LineTo(temp);
		}
		LineTo(p.AddY(cs));
		if(cs > 0.0f)
			AddArc(p, FPoint(cs2), SMathConst::Pi_f * 1.5f, SMathConst::Pi_f * 2.0f - ellipseAngularIncrement);
		CloseSubPath();
	}
}

void GeomPath::AddPath(const GeomPath & other)
{
	for(uint i = 0; i < other.getCount();) {
		const float type = other.get(i++);
		if(type == moveMarker) {
			StartNewSubPath(other.getPoint(i));
			i += 2;
		}
		else if(type == lineMarker) {
			LineTo(other.getPoint(i));
			i += 2;
		}
		else if(type == quadMarker) {
			QuadraticTo(other.getPoint(i), other.getPoint(i + 2));
			i += 4;
		}
		else if(type == cubicMarker) {
			CubicTo(other.getPoint(i), other.getPoint(i + 2), other.getPoint(i + 4));
			i += 6;
		}
		else if(type == closeSubPathMarker) {
			CloseSubPath();
		}
		else {
			// something's gone wrong with the element list!
			assert(0);
		}
	}
}

void GeomPath::AddPath(const GeomPath & other, const AffineTransform & transformToApply)
{
	for(uint i = 0; i < other.getCount();) {
		const float type = other.get(i++);
		if(type == closeSubPathMarker) {
			CloseSubPath();
		}
		else {
			FPoint p = other.getPoint(i);
			i += 2;
			transformToApply.TransformPoint(&p);
			if(type == moveMarker)
				StartNewSubPath(p);
			else if(type == lineMarker)
				LineTo(p);
			else if(type == quadMarker) {
				FPoint p2 = other.getPoint(i);
				i += 2;
				transformToApply.TransformPoint(&p2);
				QuadraticTo(p, p2);
			}
			else if(type == cubicMarker) {
				FPoint p2 = other.getPoint(i);
				i += 2;
				FPoint p3 = other.getPoint(i);
				i += 2;
				transformToApply.TransformPoint(&p2);
				transformToApply.TransformPoint(&p3);
				CubicTo(p, p2, p3);
			}
			else {
				// something's gone wrong with the element list!
				assert(0);
			}
		}
	}
}

//==============================================================================
void GeomPath::ApplyTransform(const AffineTransform & transform)
{
	PathMin = ZEROFPOINT;
	PathMax = ZEROFPOINT;
	bool   setMaxMin = false;
	for(uint i = 0; i < getCount();) {
		const float type = get(i++);
		if(type == moveMarker) {
			FPoint & r_p = *(FPoint *)SArray::at(i);
			transform.TransformPoint(&r_p);
			if(setMaxMin) {
				PathMin = fmin(PathMin, r_p);
				PathMax = fmax(PathMax, r_p);
			}
			else {
				PathMin = PathMax = r_p;
				setMaxMin = true;
			}
			i += 2;
		}
		else if(type == lineMarker) {
			FPoint & r_p = *(FPoint *)SArray::at(i);
			transform.TransformPoint(&r_p);
			PathMin = fmin(PathMin, r_p);
			PathMax = fmax(PathMax, r_p);
			i += 2;
		}
		else if(type == quadMarker) {
			FPoint & r_p1 = *(FPoint *)SArray::at(i);
			FPoint & r_p2 = *(FPoint *)SArray::at(i+2);
			transform.TransformPoint(&r_p1);
			transform.TransformPoint(&r_p2);
			PathMin = fmin(fmin(PathMin, r_p1), r_p2);
			PathMax = fmax(fmax(PathMax, r_p1), r_p2);
			i += 4;
		}
		else if(type == cubicMarker) {
			FPoint & r_p1 = *(FPoint *)SArray::at(i);
			FPoint & r_p2 = *(FPoint *)SArray::at(i+2);
			FPoint & r_p3 = *(FPoint *)SArray::at(i+4);
			transform.TransformPoint(&r_p1);
			transform.TransformPoint(&r_p2);
			transform.TransformPoint(&r_p3);
			PathMin = fmin(fmin(fmin(PathMin, r_p1), r_p2), r_p3);
			PathMax = fmax(fmax(fmax(PathMax, r_p1), r_p2), r_p3);
			i += 6;
		}
	}
}

const AffineTransform GeomPath::GetTransformToScaleToFit(
	FPoint lu, FPoint size, int preserveProportions, const Justification & justification) const
{
	FPoint s, sz;
	GetBounds(&s, &sz);
	AffineTransform at;
	if(preserveProportions) {
		if(size.X <= 0 || size.Y <= 0 || sz.X <= 0 || sz.Y <= 0)
			at = AffineTransform::identity;
		else {
			FPoint new_size;
			const float src_ratio = sz.Ratio();
			if(src_ratio > size.Ratio())
				new_size.Set(size.Y / src_ratio, size.Y);
			else
				new_size.Set(size.X, size.X * src_ratio);
			FPoint new_centre = lu;
			if(justification.testFlags(Justification::left))
				new_centre.X += new_size.X * 0.5f;
			else if(justification.testFlags(Justification::right))
				new_centre.X += size.X - new_size.X * 0.5f;
			else
				new_centre.X += size.X * 0.5f;
			if(justification.testFlags(Justification::top))
				new_centre.Y += new_size.Y * 0.5f;
			else if(justification.testFlags(Justification::bottom))
				new_centre.Y += size.Y - new_size.Y * 0.5f;
			else
				new_centre.Y += size.Y * 0.5f;
			at = AffineTransform::Translation((sz * -0.5f) - s).Scaled(new_size / sz).Translated(new_centre);
		}
	}
	else
		at = AffineTransform::Translation(s.Neg()).Scaled(size / sz).Translated(lu);
	return at;
}

//==============================================================================
static const float collisionDetectionTolerence = 20.0f;

int GeomPath::Contains(FPoint p) const
{
	int    yes = 0;
	if(p > PathMin && p < PathMax) {
		PathFlatteningIterator i(*this, AffineTransform::identity, collisionDetectionTolerence);
		int    positiveCrossings = 0;
		int    negativeCrossings = 0;
		while(i.Next())
			if((i.p1.Y <= p.Y && i.p2.Y > p.Y) || (i.p2.Y <= p.Y && i.p1.Y > p.Y)) {
				const float intersectX = i.p1.X +(i.p2.X - i.p1.X) *(p.Y - i.p1.Y) / (i.p2.Y - i.p1.Y);
				if(intersectX <= p.X)
					if(i.p1.Y < i.p2.Y)
						++positiveCrossings;
					else
						++negativeCrossings;
			}
		yes = useNonZeroWinding ? (negativeCrossings != positiveCrossings) : ((negativeCrossings + positiveCrossings) & 1) != 0;
	}
	return yes;
}

int GeomPath::IntersectsLine(FPoint a, FPoint b)
{
	PathFlatteningIterator i(*this, AffineTransform::identity, collisionDetectionTolerence);
	const FLine line1(a, b);
	while(i.Next()) {
		const FLine line2(i.p1, i.p2);
		FPoint i;
		if(line1.Intersects(line2, &i))
			return 1;
	}
	return 0;
}

const GeomPath GeomPath::CreatePathWithRoundedCorners(float cornerRadius) const
{
    if(cornerRadius <= 0.01f)
        return *this;
    int indexOfPathStart = 0, indexOfPathStartThis = 0;
    bool lastWasLine = false, firstWasLine = false;
    GeomPath p;
	for(uint n = 0; n < getCount();) {
        const float type = get(n++);
        if(type == moveMarker) {
            indexOfPathStart = p.getCount();
            indexOfPathStartThis = n - 1;
            p.StartNewSubPath(getPoint(n));
			n += 2;
            lastWasLine = false;
            firstWasLine =(get(n) == lineMarker);
        }
        else if(type == lineMarker || type == closeSubPathMarker) {
			FPoint start = ZEROFPOINT, end = ZEROFPOINT, join = ZEROFPOINT;
            if(type == lineMarker) {
				end = getPoint(n);
				n += 2;
                if(n > 8) {
					start = getPoint(n-8);
					join  = getPoint(n-5);
                }
            }
            else {
				end = getPoint(indexOfPathStartThis + 1);
                if(n > 6) {
					start = getPoint(n-6);
					join  = getPoint(n-3);
                }
            }
            if(lastWasLine) {
                const double len1 = (start - join).Hypot();
                if(len1 > 0) {
                    const double propNeeded = MIN(0.5, cornerRadius / len1);
					*(FPoint *)p.SArray::at(p.getCount() - 2) = join - (join - start) * propNeeded;
                }
                const double len2 = (end - join).Hypot();
                if(len2 > 0) {
                    const double propNeeded = MIN(0.5, cornerRadius / len2);
                    p.QuadraticTo(join, join + (end - join) * propNeeded);
                }
                p.LineTo(end);
            }
            else if(type == lineMarker) {
                p.LineTo(end);
                lastWasLine = true;
            }
            if(type == closeSubPathMarker) {
                if(firstWasLine) {
                    start = getPoint(n - 3);
					join = end;
                    end  = getPoint(indexOfPathStartThis + 4);
                    const double len1 = (start - join).Hypot();
                    if(len1 > 0) {
                        const double propNeeded = MIN(0.5, cornerRadius / len1);
						*(FPoint *)p.SArray::at(p.getCount() - 2) = join - (join - start) * propNeeded;
                    }
                    const double len2 = (end - join).Hypot();
                    if(len2 > 0) {
                        const double propNeeded = MIN(0.5, cornerRadius / len2);
						end = join + (end - join) * propNeeded;
                        p.QuadraticTo(join, end);
						*(FPoint *)p.SArray::at(indexOfPathStart + 1) = end;
                    }
                }
                p.CloseSubPath();
            }
        }
        else if(type == quadMarker) {
            lastWasLine = false;
			p.QuadraticTo(getPoint(n), getPoint(n+2));
			n += 4;
        }
        else if(type == cubicMarker) {
            lastWasLine = false;
			p.CubicTo(getPoint(n), getPoint(n+2), getPoint(n+4));
			n += 6;
        }
    }
    return p;
}

int GeomPath::Read(SBuffer & rBuf)
{
	int    ok = -1;
	FPoint p1, p2, p3;
	while(ok < 0 && rBuf.GetAvailableSize()) {
		char   tag;
		rBuf.Read(tag);
		switch(tag) {
			case 'm':
				p1.Read(rBuf);
				StartNewSubPath(p1);
				break;
			case 'l':
				p1.Read(rBuf);
				LineTo(p1);
				break;
			case 'q':
				p1.Read(rBuf);
				p2.Read(rBuf);
				QuadraticTo(p1, p2);
				break;
			case 'b':
				p1.Read(rBuf);
				p2.Read(rBuf);
				p3.Read(rBuf);
				CubicTo(p1, p2, p3);
				break;
			case 'c':
				CloseSubPath();
				break;
			case 'n':
				useNonZeroWinding = true;
				break;
			case 'z':
				useNonZeroWinding = false;
				break;
			case 'e': // end of path marker
				ok = 1;
				break;
			default:
				ok = 0;
				break;
		}
	}
	return ok;
}

int GeomPath::Write(SBuffer & rBuf) const
{
	int    ok = 1;
	char   tag = useNonZeroWinding ? 'n' : 'z';
	rBuf.Write(tag);
	for(uint i = 0; i < getCount();) {
        const float type = get(i++);
        if(type == moveMarker) {
			rBuf.Write(tag = 'm');
			getPoint(i).Write(rBuf);
			i += 2;
        }
        else if(type == lineMarker) {
			rBuf.Write(tag = 'l');
			getPoint(i).Write(rBuf);
			i += 2;
        }
        else if(type == quadMarker) {
			rBuf.Write(tag = 'q');
			getPoint(i).Write(rBuf);
			getPoint(i + 2).Write(rBuf);
			i += 4;
        }
        else if(type == cubicMarker) {
			rBuf.Write(tag = 'b');
			getPoint(i).Write(rBuf);
			getPoint(i + 2).Write(rBuf);
			getPoint(i + 4).Write(rBuf);
			i += 6;
        }
        else if(type == closeSubPathMarker)
			rBuf.Write(tag = 'c');
    }
	rBuf.Write(tag = 'e'); // marks the end-of-path
	return ok;
}

int GeomPath::ToString(SString & rBuf) const
{
	int    ok = 1;
    if(!useNonZeroWinding)
		rBuf.CatChar('a').Space();
    float  lastMarker = 0.0f;
	for(uint i = 0; i < getCount();) {
        const  float marker = get(i++);
		char   markerChar = 0;
        int    numCoords = 0;
        if(marker == moveMarker) {
            markerChar = 'm';
            numCoords = 2;
        }
        else if(marker == lineMarker) {
            markerChar = 'l';
            numCoords = 2;
        }
        else if(marker == quadMarker) {
            markerChar = 'q';
            numCoords = 4;
        }
        else if(marker == cubicMarker) {
            markerChar = 'c';
            numCoords = 6;
        }
        else {
            assert(marker == closeSubPathMarker);
            markerChar = 'z';
        }
        if(marker != lastMarker) {
			rBuf.CatChar(markerChar).Space();
            lastMarker = marker;
        }
        while(--numCoords >= 0 && i < getCount())
			rBuf.Cat(get(i++), NMBF_NOTRAILZ).Space();
    }
	rBuf.CatChar('$');
	return ok;
}

static int NextToken(SStrScan & rScan, SString & rToken)
{
	int    ok = 0;
	rScan.Skip();
	if(rScan.SearchChar(' ')) {
		rScan.Get(rToken);
		rScan.IncrLen();
		ok = 1;
	}
	return ok;
}

static int GetPoints(SStrScan & rScan, uint count, FPoint * pList)
{
	int    ok = 1;
	SString token;
	for(uint i = 0; i < count; ++i) {
		THROW(NextToken(rScan, token));
		pList[i].X = (float)atof(token);
		THROW(NextToken(rScan, token));
		pList[i].Y = (float)atof(token);
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}

int GeomPath::FromString(const char * pBuf)
{
	int    ok = -1;
	Clear();
	SetUsingNonZeroWinding(true);
	FPoint points[3];
	SStrScan scan(pBuf);
	SString token;
	while(ok < 0) {
		THROW(NextToken(scan, token));
		switch(*token) {
			case '$': // Завершение пути
				ok = 1;
				break;
			case 'm':
				THROW(GetPoints(scan, 1, points));
				StartNewSubPath(points[0]);
				break;
			case 'l':
				THROW(GetPoints(scan, 1, points));
				LineTo(points[0]);
				break;
			case 'q':
				THROW(GetPoints(scan, 2, points));
				QuadraticTo(points[0], points[1]);
				break;
			case 'c':
				THROW(GetPoints(scan, 3, points));
				CubicTo(points[0], points[1], points[2]); break;
				break;
			case 'z':
				CloseSubPath();
				break;
			case 'a':
				SetUsingNonZeroWinding(false);
				break;
			default:
				CALLEXCEPT();
				break;
		}
	}
	CATCH
		ok = 0;
	ENDCATCH
	return ok;
}
//
//
//
GeomPath::Iterator::Iterator(const GeomPath & path_) : path(path_)
{
	Index = 0;
}

int GeomPath::Iterator::Next()
{
	int    ok = 1;
	if(Index < path.getCount()) {
		const float type = path.get(Index++);
		if(type == moveMarker) {
			elementType = StartNewSubPath;
			p1 = path.getPoint(Index);
			Index += 2;
		}
		else if(type == lineMarker) {
			elementType = lineTo;
			p1 = path.getPoint(Index);
			Index += 2;
		}
		else if(type == quadMarker) {
			elementType = quadraticTo;
			p1 = path.getPoint(Index);
			p2 = path.getPoint(Index+2);
			Index += 4;
		}
		else if(type == cubicMarker) {
			elementType = cubicTo;
			p1 = path.getPoint(Index);
			p2 = path.getPoint(Index+2);
			p3 = path.getPoint(Index+4);
			Index += 6;
		}
		else if(type == closeSubPathMarker)
			elementType = closePath;
	}
	else
		ok = 0;
	return ok;
}

