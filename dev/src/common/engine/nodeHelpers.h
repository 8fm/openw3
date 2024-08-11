
#pragma once

// Function gets the transformation matrix from local node space to global space.
// Takes into consideration all special cases for particular node types.
Matrix GetNodeLocalToWorldMatrix( CNode* node );
