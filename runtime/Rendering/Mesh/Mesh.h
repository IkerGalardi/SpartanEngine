/*
Copyright(c) 2016-2022 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//= INCLUDES =====================
#include <vector>
#include "../RHI/RHI_Definition.h"
#include "../RHI/RHI_Vertex.h"
//================================

namespace Spartan
{
    class Mesh
    {
    public:
        Mesh() = default;
        ~Mesh() = default;

        // Geometry
        void Clear();
        void GetGeometry(
            uint32_t indexOffset,
            uint32_t indexCount,
            uint32_t vertexOffset,
            uint32_t vertexCount,
            std::vector<uint32_t>* indices,
            std::vector<RHI_Vertex_PosTexNorTan>* vertices
        );
        uint32_t GetMemoryUsage() const;

        // Add geometry
        void AddVertices(const std::vector<RHI_Vertex_PosTexNorTan>& vertices, uint32_t* vertex_offset_out);
        void AddIndices(const std::vector<uint32_t>& indices, uint32_t* index_offset_out);
        // Get geometry
        std::vector<RHI_Vertex_PosTexNorTan>& GetVertices() { return m_vertices; }
        std::vector<uint32_t>& GetIndices()                 { return m_indices; }
        // Get counts
        uint32_t GetVertexCount() const;
        uint32_t GetIndexCount() const;

        void Optimize();

    private:
        std::vector<RHI_Vertex_PosTexNorTan> m_vertices;
        std::vector<uint32_t> m_indices;
        std::mutex m_mutex_add_indices;
        std::mutex m_mutex_add_verices;
    };
}
