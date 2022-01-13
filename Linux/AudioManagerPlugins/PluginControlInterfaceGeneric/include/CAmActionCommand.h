/******************************************************************************
 * @file: CAmActionCommand.h
 *
 * This file contains the declaration of Action command class (member functions
 * and data members) used to implement the leaf action class.
 * Only the leaf actions can perform asynchronous operations.
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

#ifndef GC_ACTIONCOMMAND_H_
#define GC_ACTIONCOMMAND_H_

#include <string>
#include <map>
#include "IAmAction.h"
#include "CAmTypes.h"
#include "CAmTimerEvent.h"

namespace am {
namespace gc {

class IAmActionParam;
/**
 * This is the base class for the leaf level actions.
 *
 * The leaf level or lowest level of action do the real job of sending the routing side
 * request and waiting for the callback acknowledgment. In general asynchronous
 * operations are possible in the these actions.
 *
 * This class is derived from the generic IAmActionCommand Class. The concrete leaf actions
 * should be derived from this class. In order to simplify the task of concrete action
 * development the action state and the update management is handled in this class.
 *
 * The idea is as below
 * - The four basic operation for an action are execute, update, undo and cleanup.
 * - These are implemented in this class and additionally _execute, _update, _undo and
 * _cleanup are defined as virtual protected. The derived concrete classes may override
 * these implementation.
 * This relieves the concrete action class implementer from the action state management.
 */
class CAmActionCommand : public IAmActionCommand
{

public:
    CAmActionCommand(const std::string& name = "");
    virtual ~CAmActionCommand();

    /**
     * @brief This function executes the action.
     *
     * @return int negative value means action resulted in asynchronous operation but the
     * result is pending, positive value means that action completed. In positive values
     * zero means the success and non zero positive value means an error occured.
     *
     */
    int execute(void);
    /**
     * @brief This function is used to get the present error of an action.
     *
     * @return int zero means no error and non zero value means erro.
     */
    int getError(void) const;
    /**
     * @brief This function is used to set the action error.
     *
     * @param error The new error value to be set.
     */
    void setError(const int error);
    /**
     * @brief This function performs the actions cleanup.
     *
     * @return int zero on success and vice versa.
     */
    int cleanup(void);
    /**
     * @brief This function returns the name of action.
     *
     *
     * @return string The name of an action.
     */
    std::string getName(void) const;
    /**
     * @brief This function gets the status of the action.
     *
     * @return ActionState The state of the action.
     */
    ActionState_e getStatus(void) const;
    /**
     * @brief This function is used to state the action state.
     *
     * @param state This new state for an action.
     */
    void setStatus(const ActionState_e state);
    /**
     * @brief This function is used to set the parent for a given action.
     *
     * @param command This is the pointer to the parent action
     * return int 0 on successful completion of the function or otherwise.
     */
    int setParent(IAmActionCommand* command);
    /**
     * @brief This function is used to set the Parameter for an action.
     *
     * @param pParam The pointer to the base parameter.
     * @param paramName The name of the parameter
     *
     * @return true on success and vice versa.
     */
    bool setParam(const std::string& paramName, IAmActionParam* pParam);
    /**
     * @brief This function is used to get the Parameter for an action.
     *
     * @param paramName The name of the parameter
     *
     * @return IAmActionParam* pointer to the base action param. NULL if invalid
     */

    IAmActionParam* getParam(const std::string& paramName);
    /**
     * @brief This function is used to perform the undo.
     *
     * @return int 0 on success and vice versa.
     */
    int undo(void);

    bool getUndoRequired(void);
    void setUndoRequried(const bool undoRequired);
    /**
     * @brief This function is used to update the completion of the child action or the
     * completion of undo operation.
     *
     * @param result This is the result of the child action completion.
     * @return int 0 on successful completion of the function or otherwise
     */
    int update(const int result);
    void setTimeout(uint32_t timeout);
    uint32_t getTimeout(void);
    uint32_t getExecutionTime(void);
    uint32_t getUndoTime(void);
protected:
    /**
     * @brief The default implementation for the _execute.
     *
     * @return negative value means pending operation and positive means execution
     * completed
     */

    virtual int _execute(void);
    /**
     * @brief The default implementation for the  undo.
     *
     * @return negative value means pending operation and positive means execution
     */

    virtual int _undo(void);
    /**
     * @brief The default implementation for the update.
     *
     * @ Result
     * @return zero means success and non zero means error
     */
    virtual int _update(const int result);
    /**
     * @brief The default implementation for the cleanup
     *
     * @return int zero means success and vice-versa
     */
    virtual int _cleanup(void);
    /**
     * @brief This function is used to add the parameters for an action.
     *
     * In the constructor of the concrete action, the action should append all the valid
     * parameters. Internally a map of action name against the action param is saved.
     *
     * @param paramName The name of the parameter.
     * @param pParam The pointer to the action parameter.
     */
    void _registerParam(const std::string& paramName, IAmActionParam* pParam);
    virtual void _timeout(void);

private:
    uint32_t _calculateTimeDifference(timespec startTime);
    // Name of the action, used mostly for debug purpose.
    std::string mName;
    // Map for storing the action parameter against parameter name
    std::map<std::string, IAmActionParam* > mMapParameters;
    // The current state of the action
    ActionState_e mStatus;
    // The error state of the action, 0 means no error and non zero means error.
    int mError;
    IAmActionCommand* mParent;
    bool mUndoRequired;
    Tcallback<CAmActionCommand > mpTimerCallback;
    uint32_t mTimeout;
    sh_timerHandle_t mTimerHandle;
    void timeout(void* data);
    timespec mStartTime;
    uint32_t mExecutionTime;
    uint32_t mUndoTime;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_ACTIONCOMMAND_H_ */
