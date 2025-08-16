#include "SubMesh.h"
#include "PassSystem.h"

namespace rendering
{
    void SubMesh::Destroy()
    {
        passMeshManager->DereferenceMesh(meshIndex);
    }
}
