#ifndef _fsm_fsm_h
#define _fsm_fsm_h

// Local Includes
#include "state.h"

// FSM Class
class CFSM
{
	protected:
		CState *		m_pCurrentState;	// Current State
		CState *		m_pNewState;		// New State
		CStateTemplate<CFSM>	m_stateInitial;		// Initial State

	public:
		// Constructor
		CFSM();

		// Destructor
		virtual ~CFSM() {}

		// Global Functions
		virtual void Update();

		// State Functions
		bool IsState(CState &State) const;
		bool GotoState(CState &NewState);

		virtual void BeginStateInitial() {}
		virtual void StateInitial() {}
		virtual void EndStateInitial() {}
};

#endif
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
