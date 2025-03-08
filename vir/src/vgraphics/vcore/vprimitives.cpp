#include "vpch.h"

namespace vir
{

GeometricPrimitive::~GeometricPrimitive()
{
    if (vertexArray_ != nullptr)
    {
        vertexArray_->unbind();
        delete vertexArray_;
        vertexArray_ = nullptr;
    }
    if (vertexBuffer_ != nullptr)
    {
        vertexBuffer_->unbind();
        delete vertexBuffer_;
        vertexBuffer_ = nullptr;
    }
    if (indexBuffer_ != nullptr)
    {
        indexBuffer_->unbind();
        delete indexBuffer_;
        indexBuffer_ = nullptr;
    }
}

//------------------------------------------------------------------------------

Quad::Quad(float width, float height, float depth):
width_(width),
height_(height),
depth_(depth)
{
    vertexArray_ = VertexArray::create();
    float x = width/2.0;
    float y = height/2.0;
    float z = depth;
    float vertices[] = { // Embedded texture coordinates for simplicity
         x,  y,  z,  1,  1,
         x, -y,  z,  1,  0,
        -x, -y,  z,  0,  0,
        -x,  y,  z,  0,  1
    };
    vertexBuffer_ = VertexBuffer::create(vertices, sizeof(vertices));
    vertexBuffer_->setLayout
    (
        {
            {"position", Shader::Float3()},
            {"textureCoordinates", Shader::Float2()}
        }
    );
    vertexArray_->bindVertexBuffer(vertexBuffer_);
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0,
    };
    indexBuffer_ = IndexBuffer::create(indices, sizeof(indices));
    vertexArray_->bindIndexBuffer(indexBuffer_);
}

void Quad::update(float width, float height, float depth)
{
    if (width == width_ && height == height_ && depth == depth_)
        return;
    float x = width/2.0;
    float y = height/2.0;
    float z = depth;
    float vertices[] = { // Embedded texture coordinates for simplicity
         x,  y,  z,  1,  1,
         x, -y,  z,  1,  0,
        -x, -y,  z,  0,  0,
        -x,  y,  z,  0,  1
    };
    vertexArray_->bind();
    vertexBuffer_->updateVertices(vertices, sizeof(vertices));
    width_ = width;
    height_ = height;
    depth_ = depth;
}

//------------------------------------------------------------------------------

TiledQuad::TiledQuad
(
    float width, 
    float height, 
    float depth, 
    uint32_t nTilesX, 
    uint32_t nTilesY
):
width_(width),
height_(height),
depth_(depth),
nTilesX_(nTilesX),
nTilesY_(nTilesY)
{
    vertexArray_ = VertexArray::create();
    calculateVertices(width, height, depth, nTilesX, nTilesY);
    updateBuffers(nTilesX, nTilesY);
}

void TiledQuad::calculateVertices
(
    float width, 
    float height, 
    float depth, 
    uint32_t nTilesX, 
    uint32_t nTilesY
)
{
    // Each vertex is of size 5: 3 components for the actual x, y, z vertex
    // coordinates plus the tc, ty texture coordinates of each vertex in
    // the 2-D x-y plane (like in the regular vir::Quad)
    vertices_.resize(5*(1+nTilesX)*(1+nTilesY));
    float z = depth;
    float dx = width/nTilesX;
    float dy = height/nTilesY;
    int k = 0;
    for (uint32_t j=0; j<nTilesY+1; j++)
    {
        float y = j*dy;
        for (uint32_t i=0; i<nTilesX+1; i++)
        {
            float x = i*dx;
            vertices_[k++] = x - width/2.0;
            vertices_[k++] = y - height/2.0;
            vertices_[k++] = z;
            vertices_[k++] = x/width;   // tc
            vertices_[k++] = y/height;  // ty
        }
    }
}

void TiledQuad::updateBuffers(uint32_t nTilesX, uint32_t nTilesY)
{
    if (vertexArray_ == nullptr)
        return;
    
    nTilesX_ = nTilesX;
    nTilesY_ = nTilesY;

    if (vertexBuffer_ != nullptr)
    {
        vertexBuffer_->unbind();
        delete vertexBuffer_;
    }
    vertexBuffer_ = VertexBuffer::create
    (
        vertices_.data(), 
        vertices_.size()*sizeof(float)
    );
    vertexBuffer_->setLayout
    (
        {
            {"position", Shader::Float3()},
            {"textureCoordinates", Shader::Float2()}
        }
    );
    vertexArray_->bindVertexBuffer(vertexBuffer_);

    if (indexBuffer_ == nullptr)
    {
        indexBuffer_ = IndexBuffer::create(nullptr, 6*sizeof(uint32_t));
        vertexArray_->bindIndexBuffer(indexBuffer_);
    }
    selectVisibleTiles(0, 0, nTilesX-1, nTilesY-1);
}

void TiledQuad::update(uint32_t nTilesX, uint32_t nTilesY)
{
    update(width_, height_, depth_, nTilesX, nTilesY);
}

void TiledQuad::update(float width, float height, float depth)
{
    update(width, height, depth, nTilesX_, nTilesY_);
}

void TiledQuad::update
(
    float width, 
    float height, 
    float depth, 
    uint32_t nTilesX, 
    uint32_t nTilesY
)
{
    if 
    (
        width == width_ && 
        height == height_ && 
        depth == depth_ && 
        nTilesX == nTilesX_ && 
        nTilesY == nTilesY_
    )
        return;
    calculateVertices(width, height, depth, nTilesX, nTilesY);
    if (nTilesX != nTilesX_ || nTilesY != nTilesY_)
    {
        updateBuffers(nTilesX, nTilesY);
    }
    else
    {
        vertexArray_->bind();
        vertexBuffer_->updateVertices
        (
            vertices_.data(), 
            vertices_.size()*sizeof(float)
        );
    }
    width_ = width;
    height_ = height;
    depth_ = depth;
}

void TiledQuad::selectVisibleTile
(
    uint32_t ti, 
    uint32_t tj
)
{
    selectVisibleTiles(ti, tj, ti, tj);
}

void TiledQuad::selectVisibleTiles
(
    uint32_t ti0, 
    uint32_t tj0, 
    uint32_t ti1, 
    uint32_t tj1
)
{
    /*  (ti0, tj0) -> v0
        (ti1, tj0) -> v1 
        (ti0, tj1) -> v2
        (ti1, tj1) -> v3

        ^ j
        |       v2 --- v3
        |        |  /  |
        |       v0 --- v1 
        |
        0---------------> i
    */
    
    uint32_t v0 = ti0 + tj0*(nTilesX_+1);
    uint32_t v1 = v0 + (ti1-ti0+1);
    uint32_t v2 = v0 + (tj1-tj0+1)*(nTilesX_+1);
    uint32_t v3 = v2 + (ti1-ti0+1);
    unsigned int indices[] = {
        v0, v1, v3,
        v0, v3, v2,
    };
    vertexArray_->bind();
    indexBuffer_->updateIndices
    (
        indices, 
        sizeof(indices)
    );
}

}