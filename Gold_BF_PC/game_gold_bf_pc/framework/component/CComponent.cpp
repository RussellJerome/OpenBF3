#include "CComponent.h"

void CComponentList::Free()
{
    for (int i = 0; i < m_numComponents; i++)
    {
        SComponentAndName& entry = m_componentsAndNames[i];
        if (entry.component)
        {
            entry.component->~CTickingComponent();
            entry.component = nullptr;
        }
    }
    m_numComponents = 0;
}

CTickingComponent* CComponentList::GetComponentSelf(unsigned int inComponentType)
{
    CTickingComponent* result = nullptr;

    for (int i = 0; i < m_numComponents; i++)
    {
        CTickingComponent* comp = m_componentsAndNames[i].component;
        if (comp && comp->IsOfObjectType(inComponentType))
            result = comp;
    }

    return result;
}

void CComponentList::TickingComponentTick(float dt)
{
    for (int i = 0; i < m_numComponents; i++)
    {
        CTickingComponent* comp = m_componentsAndNames[i].component;
        if (comp)
            comp->TickingComponentTick(dt);
    }
}

void CComponentList::OnCollision(CGameProp* prop, CollisionInfo* info)
{
    for (int i = 0; i < m_numComponents; i++)
    {
        CTickingComponent* comp = m_componentsAndNames[i].component;
        if (comp)
            comp->OnCollision(prop, info);
    }
}

void CComponentList::TickingComponentCalcMatrices(float dt)
{
    for (int i = 0; i < m_numComponents; i++)
    {
        CTickingComponent* comp = m_componentsAndNames[i].component;
        if (comp)
            comp->CalcMatrices(m_prop, dt);
    }
}