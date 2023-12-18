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

class Quad : public GeometricPrimitive
{
protected:
    glm::vec3 widthHeightDepth_;
public:
    Quad(float, float, float);

    // Re-draw with given width, height, depth
    void update(float, float, float);
    const glm::vec3& getWidthHeightDepth(){return widthHeightDepth_;}
};

}

#endif