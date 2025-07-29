#include "SubMesh.h"
#include "PassMeshManager.h"

namespace rendering
{
    void SubMesh::Destroy()
    {
        passMeshManager->DereferenceMesh(meshIndex);
    }
}
