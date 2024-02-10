#ifndef V_GEOMETRIC_PRIMITIVES_H
#define V_GEOMETRIC_PRIMITIVES_H

namespace vir
{

class VertexArray;
class VertexBuffer;
class IndexBuffer;

class GeometricPrimitive
{
protected:
    VertexArray* vertexArray_;
    VertexBuffer* vertexBuffer_;
    IndexBuffer* indexBuffer_;
    GeometricPrimitive() = default;
public:
    virtual ~GeometricPrimitive();
    VertexArray* vertexArray() {return vertexArray_;}
};

// Class to manage a quad that is always perpendicular to the Z axis of a
// right-handed reference frame, wherein its width is along the X axis and
// height along the Y axis
class Quad : public GeometricPrimitive
{
protected:
    float width_;
    float height_;
    float depth_;
public:
    Quad(float, float, float);

    // Re-draw with given width, height, depth
    void update(float, float, float);
    const float& width() const {return width_;}
    const float& height() const {return height_;}
    const float& depth() const {return depth_;}
};

}

#endif