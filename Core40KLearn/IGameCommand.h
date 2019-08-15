#pragma once


#include "Utility.h"
#include <memory>


namespace c40kl
{


enum class CommandType
{
	UNIT_ORDER,
	END_PHASE
};


//Forward definition
class GameState;


/// <summary>
/// A game command is something which modifies the game
/// state, possibly resulting in a distribution of resulting
/// game states. Note that, since GameState objects are
/// immutable, commands always make copies of the states.
/// </summary>
class C40KL_API IGameCommand
{
public:
	virtual ~IGameCommand() = default;

	/// <summary>
	/// Apply this command to the given game state, to
	/// produce a distribution of resulting states. Note
	/// that you will have obtained this command object
	/// from a list of commands given directly by the game
	/// state; if you apply this command to a different
	/// game state, it may fail, because the action may
	/// not be possible.
	/// </summary>
	/// <param name="state">The input state to apply the action to.</param>
	/// <param name="outStates">The vector to push all resulting states to.</param>
	/// <param name="outDistribution">The vector to push the corresponding probabilities to.</param>
	virtual void Apply(const GameState& state, std::vector<GameState>& outStates,
		std::vector<float>& outDistribution) const = 0;

	/// <summary>
	/// Check if this command will perform the same operation
	/// given the same input world state (and result in the
	/// same state distribution).
	/// </summary>
	/// <param name="cmd">The command to check equality with.</param>
	/// <returns>True if and only if the two objects are equal.</returns>
	virtual bool Equals(const IGameCommand& cmd) const = 0;

	/// <summary>
	/// Convert the given command to a human-readable representation.
	/// Note that, since game commands do not know what state they
	/// will be applied to, they do not know what units they operate
	/// on, only the necessary information to find those units (like
	/// position.)
	/// </summary>
	/// <returns>This command in a human-readable string representation.</returns>
	virtual String ToString() const = 0;

	/// <summary>
	/// To help distinguish between the different kinds of commands,
	/// a type enum is used.
	/// </summary>
	/// <returns>The type of this command object.</returns>
	virtual CommandType GetType() const = 0;
};


/// <summary>
/// Many commands are "unit order commands", meaning they
/// ask a unit to do something. Unit order commands always
/// have a source position (which unit is acting?) and a
/// target position (e.g. where are they moving to or what
/// are they shooting at?)
/// </summary>
class C40KL_API IUnitOrderCommand :
	public IGameCommand
{
public:
	virtual ~IUnitOrderCommand() = default;

	/// <summary>
	/// Get the source position (the position of the acting
	/// unit).
	/// </summary>
	/// <returns>(x,y) of the acting unit.</returns>
	virtual Position GetSourcePosition() const = 0;

	/// <summary>
	/// Get the target position (the target of the action,
	/// can either be a location or the location of an
	/// enemy unit, depending on the action itself).
	/// </summary>
	/// <returns>(x,y) of the target.</returns>
	virtual Position GetTargetPosition() const = 0;

	virtual inline CommandType GetType() const override { return CommandType::UNIT_ORDER; }
};


typedef std::shared_ptr<IGameCommand> GameCommandPtr;


class C40KL_API GameCommandArray : public std::vector<GameCommandPtr>
{ };


} // namespace c40kl


