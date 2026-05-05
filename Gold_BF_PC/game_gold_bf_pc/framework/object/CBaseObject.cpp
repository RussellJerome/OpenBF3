#include "CBaseObject.h"
#include "dictionary/CEngineSerialiseStore.h"

const char* CBaseObject::s_baseFlagsStrings[6] =
{
  "k_baseobflag_dontRuntimeSerialiseSave",
  "k_baseobflag_dontNetworkSerialiseSave",
  "k_baseobflag_hasBeenCached",
  "k_baseobflag_doNotCache",
  "k_baseobflag_objectMigrated",
  "k_baseobflag_hasNoOwner"
};

void CBaseObject::Serialise(CSerialiseStore* ioStore)
{
    // On load modes, resolve the save template from the dictionary
    //ESerialiseMode mode = ioStore->m_mode;
    //bool isLoad = (mode == k_serialiseModeLoad
    //    || mode == k_serialiseModeRuntimeLoad
    //    || mode == k_serialiseModeNetworkLoad);

    //if (isLoad)
    //    m_saveTemplate = (CTemplate*)ioStore->m_dict->GetTemplate(ioStore->m_dict);

    //ioStore->SerialiseFlagsAsIndexedStringsCoreT<unsigned char>(
    //    "baseobflags",
    //    &m_baseFlags,
    //    CBaseObject::s_baseFlagsStrings,
    //    6,
    //    true,
    //    nullptr,
    //    0x3C);
}