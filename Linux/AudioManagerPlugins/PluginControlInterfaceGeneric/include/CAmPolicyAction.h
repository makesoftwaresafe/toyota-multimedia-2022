/******************************************************************************
 * @file: CAmPolicyAction.h
 *
 * This file contains the declaration of policy action class
 * (member functions and data members)
 *
 * @component: AudioManager Generic Controller
 *
 * @author: Toshiaki Isogai <tisogai@jp.adit-jv.com>
 *          Kapildev Patel  <kpatel@jp.adit-jv.com>
 *          Prashant Jain   <pjain@jp.adit-jv.com>
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 *****************************************************************************/
#ifndef GC_POLICYACTION_H_
#define GC_POLICYACTION_H_
#include "CAmActionContainer.h"
#include "CAmTypes.h"

namespace am {
namespace gc {

class IAmPolicySend;
class CAmControlReceive;
class CAmClassElement;

class CAmPolicyAction : public CAmActionContainer
{
public:
    CAmPolicyAction(const std::vector<gc_Action_s > &listAction, IAmPolicySend* pPolicySend,
                    CAmControlReceive* pControlReceive);
    virtual ~CAmPolicyAction();
protected:
    int _execute(void);
    int _update(const int Result);
private:
    void _createActions(gc_Action_s &policyAction,
                        std::vector<IAmActionCommand * >& listFrameworkActions);
    void _getClassList(gc_Action_s &policyAction, std::vector<CAmClassElement * >& listClasses);
    void _getActions(gc_Action_s &policyAction, std::vector<IAmActionCommand * >& listFrameworks);
    std::string _getParam(std::map<std::string, std::string > &map, const std::string &elementName);
    void _setActionParameters(std::map<std::string, std::string >& mapParams,
                              IAmActionCommand *pAction);
    am_Error_e _getListNames(std::string name, std::vector<std::string >& listNames);

    std::vector<gc_Action_s > mListActions;
    IAmPolicySend* mpPolicySend;
    std::map<std::string, std::vector<am_ConnectionState_e > > mMapConnectionStates;
    CAmControlReceive* mpControlReceive;
};
}
}
#endif //GC_POLICYACTION_H_
