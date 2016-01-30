#ifndef MTAP_SHADINGNODE_H
#define MTAP_SHADINGNODE_H

#include "shadingtools/shadingnode.h"

class mtap_ShadingNode : public ShadingNode
{
public:
    mtap_ShadingNode();
    mtap_ShadingNode(ShadingNode& other);

    void translate();
};

#endif
