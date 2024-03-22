#include "MetaLayer.h"

MetaLayer::MetaLayer(BRect bounds, const char* name, int type) : Layer(bounds, name), fType(type) {}

MetaLayer::~MetaLayer() {}