/******************************************************************************
 * @file: IAmAction.h
 *
 * This file contains the declaration of abstract action class used to provide
 * the interface for container class of action
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

#ifndef GC_IACTION_H_
#define GC_IACTION_H_

#include <string>
#include "IAmEventObserver.h"
#include "CAmEventSubject.h"

namespace am {
namespace gc {

/**
 * This enumeration has all the possible states of an action
 */
enum ActionState_e
{
    /**
     * Action yet to be started
     */
    AS_NOT_STARTED,
    /**
     * Action resulted in asynchronous operations, actions has started but
     * waiting the completion
     */
    AS_EXECUTING,
    /**
     * action completed successfully
     */
    AS_COMPLETED,
    /*
     * error in action execution hence undo was started, undo resulted in async action
     * waiting undo completion
     */
    AS_UNDOING,
    /*
     * undo is complete
     */
    AS_UNDO_COMPLETE,
    /*
     * ACTION IS STOPPED BECAUSE OF AN ERROR
     */
    AS_ERROR_STOPPED
};
/**
 * This is the abstract class for action parameters.
 *
 * The behavior of an actions depends on its parameters. In order to perform any action
 * following steps are required
 * - Based on the trigger and business logic decide the set of actions
 * - Set the parameter for the actions
 * - The action framework performs actions one after another.
 */
class IAmActionParam
{
public:
    virtual ~IAmActionParam()
    {
    }
    /**
     * @brief The clone interface to set the parameters.
     */
    virtual void clone(IAmActionParam*)=0;
};

/**
 * This class is abstract class for an action.
 *
 * The operation of controller can be summarized in a line as "on a trigger it performs
 * certain a set actions". The triggers are the hook function called from control send
 * interface. The set of actions come from the business logic.
 * To give an example consider a case where radio is playing and there is a connection
 * request for traffic announcement, business logic wants to reduce the radio volume
 * and play the navigation on top. In this case trigger is the connection request for
 * traffic announcement and the action set is
 * - reduce radio volume
 * - connect traffic announcement.
 *   The above two are actions from the business logic would result in a set of routing
 *   side asynchronous operations as below
 * - async set radio sink volume.
 * - async connect route from traffic announcement source to sink, depending on the
 * route, this main connection might need one or more connections.
 * - finally change the traffic announcement source state to SS_ON if its not SS_ON.
 *
 *     To summarize, any action can be constructed using multiple child actions.
 *  The above concept is generalized using this class. On broader level two categories of
 *  actions are possible. One which are container actions which has one or many
 *  child actions. The other are leaf actions or the lowest level of actions, these actions
 *  dont have child actions.
 *  Example: Container actions: main conenction connect, which can have multiple child
 *  connect actions and source state change action.
 *           Leaf actions : Routing side set sink volume.
 */
class IAmActionCommand : public IAmEventObserver, public CAmEventSubject
{
public:
    IAmActionCommand()
    {
    }

    virtual ~IAmActionCommand()
    {
    }
    /**
     * @brief This function is used to call the execute.
     *
     * @return int negative return value means action is pending execution,
     *             positive values mean action completed execution.
     */
    virtual int execute(void) = 0;
    /**
     * @brief This function is used for undo of the procedure.
     *
     * @return int zero return means undo was successful and non zero value indicates error.
     */
    virtual int undo(void) = 0;
    /**
     * @brief This is called for cleanup of the action.
     *
     * @return int zero return means undo was successful and non zero value indicates error.
     */
    virtual int cleanup(void) = 0;
    /**
     * @brief This function is used to register the parent action. The registered parent is
     * an observer for the child action.
     *
     * @param cmd* The pointer to the parent action.
     *
     * @return int: zero on success, non zero for error.
     */
    virtual int setParent(IAmActionCommand* cmd) = 0;
    /*
     * @brief This function is used to get the status of the action.
     *
     * @return ActionState the present state of the action.
     */
    virtual ActionState_e getStatus(void) const = 0;
    /**
     * @brief This function is used to set the status of an action.
     */
    virtual void setStatus(const ActionState_e) = 0;
    /**
     * @brief This function is used to get the error of an action.
     */
    virtual int getError(void) const = 0;
    /**
     * @brief This function is used to set the error of an action.
     *
     * @return int the error of an action.
     */
    virtual void setError(const int error) = 0;

    virtual bool getUndoRequired(void) = 0;
    virtual void setUndoRequried(const bool undoRequired)=0;
    /**
     * @brief This function is used to get the action name.
     *
     * @return string The name of an action.
     */
    virtual std::string getName(void) const = 0;
    /**
     * @brief This function is used to set parameter for an action. The parameter for an
     * action is stored as a map, parameter name against the action parameter.
     *
     * @param paramName The name of the parameter
     * @param pAction The pointer to the action parameter.
     *
     * @return bool true means success and vice versa.
     */
    virtual bool setParam(const std::string& paramName, IAmActionParam* pAction)=0;
    /**
     * @brief This function is used to get the action parameter.
     *
     * @param paramName The name of the action parameter
     *
     * @return IAmActionParam* pointer to the base action param, NULL if invalid
     */
    virtual IAmActionParam* getParam(const std::string& paramName) = 0;

    virtual int update(const int result)=0;
    virtual void setTimeout(uint32_t timeout)=0;
    virtual uint32_t getTimeout(void)=0;
    virtual uint32_t getExecutionTime(void)=0;
    virtual uint32_t getUndoTime(void)=0;

};

template <class T1>
class CAmActionParam : public IAmActionParam
{
private:
    T1 mParameterValue;
    bool mValid;
public:
    CAmActionParam() :
                                    mValid(false)
    {
    }
    CAmActionParam(T1 param)
    {
        mParameterValue = param;
        mValid = true;
    }
    /**
     * @brief This function sets the parameter.
     *
     * @param param The parameter value to be set.
     */
    void setParam(T1 param)
    {
        mParameterValue = param;
        mValid = true;
    }
    /**
     * @brief This function gets the parameter.
     *
     * @param param The parameter value to be returned.
     * @return bool true if the parameter is valid and vice versa. If a getparam is called
     * before setting a parameter false is returned.
     */
    bool getParam(T1& param)
    {
        if (mValid == true)
        {
            param = mParameterValue;
        }
        return mValid;
    }
    /*
     * @brief This function clones a parameter.
     *
     * @param param Pointer to the base parameter class.
     *
     * @code
     *
     *   // On the business logic side
     *   //first create a new action parameter for example sink name for set sink volume
     *   CAmActionParam<std::string> sinkNameParam("Speaker");
     *   //Once the action parameter is created, it is set for an action
     *   // set action would clone the passed action parameter.
     *   CAmUserActionSetVolume* actionSinkSetVolume = new CAmUserActionSetVolume();
     *   actionSinkSetVolume->setActionParam("sinkname",sinkNameParam);
     *
     *   //Action during the execution gets the parameter
     *   mSinkparam.getParam(sinkName);
     * @endcode
     */
    void clone(IAmActionParam* param)
    {
        CAmActionParam<T1 > *pParam = (CAmActionParam<T1 >*)param;
        mValid = pParam->getParam(mParameterValue);
    }
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_IACTION_H_ */
