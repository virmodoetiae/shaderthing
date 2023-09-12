#include "vpch.h"

namespace vir
{

GeometricPrimitive::~GeometricPrimitive()
{
    delete vertexArray_;
    vertexArray_ = nullptr;
    delete vertexBuffer_;
    vertexBuffer_ = nullptr;
    delete indexBuffer_;
    indexBuffer_ = nullptr;
}

Quad::Quad(float width, float height, float depth):
widthHeightDepth_(width, height, depth)
{
    vertexArray_ = VertexArray::create();
    float x = width/2.0;
    float y = height/2.0;
    float z = depth;
    float vertices[] = { // Embedded texture coordinates for simplicity
        x, y, z, 1, 1,
        x, -y, z, 1, 0,
        -x, -y, z, 0, 0,
        -x, y, z, 0, 1
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
    float x = width/2.0;
    float y = height/2.0;
    float z = depth;
    float vertices[] = { // Embedded texture coordinates for simplicity
        x, y, z, 1, 1,
        x, -y, z, 1, 0,
        -x, -y, z, 0, 0,
        -x, y, z, 0, 1
    };
    vertexArray_->bind();
    vertexBuffer_->updateVertices(vertices, sizeof(vertices));
    widthHeightDepth_ = glm::vec3(width, height, depth);
}

}