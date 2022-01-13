/******************************************************************************
 * @file: CAmActionContainer.h
 *
 * This file contains the declaration of Action container class (member functions
 * and data members) used to implement the action parent classes.
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

#ifndef GC_ACTIONCONTAINER_H_
#define GC_ACTIONCONTAINER_H_

#include <map>
#include <time.h>
#include "IAmAction.h"
#include "IAmEventObserver.h"
#include "CAmEventSubject.h"
#include "CAmTypes.h"
namespace am {
namespace gc {

class IAmActionParam;
/**
 * This is the base class for the container actions.
 *
 * The container actions are the actions which are composed of sub-actions. The container action
 * could be made up of several container actions or the leaf actions. This class provides the
 * implementation for the state management of the container actions, additionally it also takes
 * care of action execution and undo sequence. In order to off-load the concrete action
 * implementation of the common tasks like state management and execution sequence the same
 * philosophy used for CAmActionCommand class is used here.
 *
 * The four function execute, undo, update and cleanup are implemented in this class and the
 * virtual protected members are _excute, _undo, _update, _cleanup provides the default
 * implementations. The concrete actions can override the default implementation. Say for example
 * a concrete actions has no operation to be performed as part of execution except calling the
 * execute of every child action, in that case it may not provide the implementation of the _execute
 * and the  _execute present in this class will be used.
 *
 */
class CAmActionContainer : public IAmActionCommand
{

public:
    CAmActionContainer(const std::string& name = "");
    virtual ~CAmActionContainer();
    /**
     * @brief The function to execute an action and all its child actions.
     *
     * The container action's own execute should never result in an asynchronous operation. In case
     * an asynchronous operation is must than this action can be broken down in to a parent and a
     * child action with child action of type CAmActionCommand and this can be made as asynchronous.
     *
     * @return zero means successfully executed and a non zero means some error occurred.
     */
    int execute(void);
    /**
     * @brief This function gets the error of an action
     *
     * @return int The error of an aciton.
     */
    int getError(void) const;
    /**
     * @brief This function is usde to set error for an action.
     *
     * @param error The new value of error for an action.
     */
    void setError(const int error);
    /**
     * @brief This function is used for cleanup of an action.
     *
     * @return int zero for successful execution and viceversa.
     */
    int cleanup(void);
    /**
     * @brief This function is used to get the name of an action
     *
     * @return string The name of the action.
     */
    std::string getName(void) const;
    /**
     * @brief This function gets the state of the action.
     *
     * @return Actionstate The state of the action.
     */
    ActionState_e getStatus(void) const;
    /**
     * @brief This function is used to set the status of an action
     *
     * @param actionState The new status of an action.
     */
    void setStatus(const ActionState_e actionState);
    /**
     * @brief This function is used to set the parent action for an action.
     *
     * @param pParentAction The pointer to the parent action.
     */
    int setParent(IAmActionCommand* pParentAction);
    /**
     * @brief This function is used to set the action parameter.
     *
     * @param paramName Name of the parameter
     * @param pParam The pointer to the action parameter
     *
     * @return bool true on success and vice-versa.
     */
    bool setParam(const std::string& paramName, IAmActionParam* pParam);
    /**
     * @brief This function gets the action parameter.
     *
     * @param paramName The name of the action parameter
     *
     * @return IAmActionParam* not NULL if valid
     */
    IAmActionParam* getParam(const std::string& paramName);
    /**
     * @brief This function is used to perform the undo for an action.
     *
     * @return int zero for successful execution and vice-versa
     */
    int undo(void);
    bool getUndoRequired(void);
    void setUndoRequried(const bool undoRequired);
    /**
     * @brief This function is to notify the action about the child action's execution or undo
     * status.
     *
     * @param result The result of the execution or undo
     *
     * @return zero for successful execution of this function and vice versa
     */
    int update(const int result);
    /**
     * @brief In order to compose container action one needs to add the child actions, this
     * function allows to add the child action.
     *
     *  @param command The pointer of the child action
     *  @return int 0 on success and vice versa.
     */
    int append(IAmActionCommand* command);
    /**
     * @brief This function allows to add an action just after the currently executing action.
     *
     *  This is required for implicit action which are not coming from the business logic. For
     *  example lets say suddenly a sink is unregistered. In this case all the connection of which
     *  sink is a part needs to be disconnected irrespective of the business logic. Such and
     *  actions are called implicit.
     *  If while some actions are in executing state, the implicit action can only execute after
     *  completion of that action.
     *
     *  @param command The pointer to the action that needs to be inserted.
     *  @return int 0 on success and vice versa.
     */
    int insert(IAmActionCommand* command);
    /**
     * @brief This function returns chcek if the action has any child actions or not
     *
     * @return bool true if no child actions and vice-versa.
     */
    bool isEmpty(void);
    void setTimeout(uint32_t timeout);
    uint32_t getTimeout(void);
    uint32_t getExecutionTime(void);
    uint32_t getUndoTime(void);
protected:
    /**
     * @brief This protected virtual function provides the default implementation for the execute.
     *
     * @return zero means successful execution, error otherwise.
     *
     */
    virtual int _execute(void);
    /**
     * @brief This function provides the default implementation of the undo.
     *
     * @return int  zero means successful execution, error otherwise.
     */
    virtual int _undo(void);
    /**
     * @brief This function provides the default cleanup implementation
     *
     * @return  int  zero means successful execution, error otherwise.
     */
    virtual int _cleanup(void);
    /**
     * @brief This function provides the default update implementation
     *
     * @param result result of the child action.
     * @return int  zero means successful execution, error otherwise.
     */
    virtual int _update(const int result);
    /**
     * @brief This function provides the default update implementation. The difference between the
     * above function and current function is, the above function is called after completion
     * of each and every child whereas the current function gets called after each child execution.
     * Same applies for the undo operation. If a concrete action needs update after execution of
     * each and every child action it should override this implementation.
     *
     * @param result The result of the child action.
     * @param index The index of the current child.
     */
    virtual int _update(const int result, const int index);

    /**
     * @brief This function is usde to append the parameter for an action.
     *
     * @param paramName The name of the parameter
     * @param pBaseParam The pointer to the action parameter.
     *
     */
    void _registerParam(const std::string& paramName, IAmActionParam* pBaseParam);
private:
    int _incrementIndex(void);
    int _decrementIndex(void);
    int _getIndex(void);
    void _setIndex(int);
    int _getNumChildActions(void);
    uint32_t _calculateTimeDifference(timespec startTime);

    // Name of the action, used mostly for debug purpose.
    std::string mName;
    // The current state of the action
    ActionState_e mStatus;
    // The index of the currentlty executing or the undoing child action.
    int mIndex;
    // The error state of the action, 0 means no error and non zero means error.
    int mError;
    // The pointer to the parent action.
    IAmActionCommand* mParent;
    // The list of the child actions
    std::vector<IAmActionCommand* > mListChildActions;
    // Map for storing the action parameter against parameter name
    std::map<std::string, IAmActionParam* > mMapParameters;
    bool mUndoRequired;
    uint32_t mTimeout;
    timespec mStartTime;
    uint32_t mExecutionTime;
    uint32_t mUndoTime;

};

} /*namespace gc */
} /* namespace am */
#endif /* GC_ACTIONCONTAINER_H_ */
