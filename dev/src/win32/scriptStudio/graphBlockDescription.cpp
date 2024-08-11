#include "build.h"

CGraphBlockDescription::CGraphBlockDescription()
{}

CGraphBlockDescription::~CGraphBlockDescription()
{}

void CGraphBlockDescription::DeInit()
{}

void CGraphBlockDescription::RegisterBlockDesc(const String &blockName, const String &desc)
{}

void CGraphBlockDescription::RegisterSocketDesc(const String &blockName, const String &socketName, const String &socketDesc)
{}

void CGraphBlockDescription::RegisterSocketDesc(const String &socketName, const String &socketDesc)
{}

void CGraphBlockDescription::GenerateHtmlDoc(const String &fname, const String &baseClass)
{}

String CGraphBlockDescription::GetTooltip(CGraphBaseItem *item)
{ return String(); }

String CGraphBlockDescription::GetTooltip(CGraphBaseItem *item, TBlocksDesc &brixDescList)
{ return String(); }

bool CGraphBlockDescription::ReadDescriptionsFromFile(const String &fname)
{ return false; }

void CGraphBlockDescription::RegisterSocketDesc(const String &blockName, const String &socketName, const String &socketDesc, TBlocksDesc &brixDescList)
{}

void CGraphBlockDescription::RegisterBlockDesc(const String &blockName, const String &desc, TBlocksDesc &brixDescList)
{}

bool CGraphBlockDescription::SaveDescriptionsDefinedInCodeToFile(const String &fname)
{ return false; }

bool CGraphBlockDescription::SaveDescriptionsDefinedInCodeToFile(const String &fname, const String &baseClass)
{ return false; }
