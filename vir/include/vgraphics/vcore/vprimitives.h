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
    VertexArray* vertexArray_   = nullptr;
    VertexBuffer* vertexBuffer_ = nullptr;
    IndexBuffer* indexBuffer_   = nullptr;
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

class TiledQuad : public GeometricPrimitive
{
protected:
    float width_;
    float height_;
    float depth_;
    uint32_t nTilesX_; // Along width direction
    uint32_t nTilesY_; // Along height direction
    std::vector<float> vertices_;
    void calculateVertices
    (
        float width, 
        float heigth, 
        float depth, 
        uint32_t nTilesX, 
        uint32_t nTilesY
    );
    void updateBuffers(uint32_t nTilesX, uint32_t nTilesY);
public:
    TiledQuad
    (
        float width, 
        float heigth, 
        float depth, 
        uint32_t nTilesX = 1, 
        uint32_t nTilesY = 1
    );
    // Update quad tiling
    void update(uint32_t nTilesX, uint32_t nTilesY);
    // Update quad dimensions
    void update(float width, float heigth, float depth);
    // Update quad dimensions and tiling
    void update
    (
        float width, 
        float heigth, 
        float depth, 
        uint32_t nTilesX, 
        uint32_t nTilesY
    );
    void selectVisibleTile
    (
        uint32_t ti, 
        uint32_t tj
    );
    // Select which range of tiles should be made visible in a rectangular region
    // starting at tile at index (ti0, tj0) and up to tile at index (ti1, tj1)
    void selectVisibleTiles
    (
        uint32_t ti0, 
        uint32_t tj0, 
        uint32_t ti1, 
        uint32_t tj1
    );
    const float& width() const {return width_;}
    const float& height() const {return height_;}
    const float& depth() const {return depth_;}
    const float& nTilesX() const {return nTilesX_;}
    const float& nTilesY() const {return nTilesY_;}
};

}

#endif