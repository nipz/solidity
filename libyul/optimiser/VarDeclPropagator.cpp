/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libyul/optimiser/VarDeclPropagator.h>
#include <libsolidity/inlineasm/AsmData.h>
#include <libdevcore/Visitor.h>
#include <algorithm>

#include <boost/algorithm/cxx11/all_of.hpp>

using namespace std;
using namespace dev;
using namespace dev::yul;

using dev::solidity::assembly::TypedName;
using dev::solidity::assembly::TypedNameList;

void VarDeclPropagator::operator()(Block& _block)
{
	m_blockScopes.push(&_block);
	ASTModifier::operator()(_block);
	m_blockScopes.pop();
}

void VarDeclPropagator::operator()(VariableDeclaration& _varDecl)
{
	if (!_varDecl.value)
	{
		m_emptyVarDecls.push_back(&_varDecl);
	}
}

void VarDeclPropagator::operator()(Assignment& _assignment)
{
	yulAssert(!_assignment.variableNames.empty(), "LHS must not be empty");

	// CASE-1
	//    var x, y, z
	//    z, x, y = RHS
	// -->
	//    var z, x, y = RHS
	if (checkAllIdentifiersMatchSame(_assignment.variableNames))
	{
		VariableDeclaration const& varDecl = getVarDecl((*begin(_assignment.variableNames)).name);
		if (varDecl.value == nullptr)
		{
			*iteratorOf(_assignment) = VariableDeclaration{
				_assignment.location,
				recreateTypedNameList(_assignment.variableNames, varDecl.variables),
				_assignment.value
			};

			removeVarDecl(varDecl);
			return;
		}
	}

	// CASE 2:
	//     var x, y
	//     x := RHS_1
	//     y := RHS_2
	// -->
	//     let x := RHS_1
	//     let y := RHS_2

	// TODO
}

/**
 * Checks whether all @p identifiers are declared in the same VariableDeclaration.
 */
bool VarDeclPropagator::checkAllIdentifiersMatchSame(vector<Identifier> const& identifiers) const
{
	yulAssert(!identifiers.empty(), "");

	VariableDeclaration const& varDecl = getVarDecl((*begin(identifiers)).name);

	return identifiers.size() == varDecl.variables.size() && all_of(
		begin(identifiers),
		end(identifiers),
		[&](Identifier const& _ident) -> bool
		{
			return find_if(
				begin(varDecl.variables),
				end(varDecl.variables),
				[&_ident](TypedName const& typedName) { return typedName.name == _ident.name; }
			) != end(varDecl.variables);
		}
	);
}

VariableDeclaration const& VarDeclPropagator::getVarDecl(std::string const& _identifier) const noexcept
{
	for (VariableDeclaration* varDecl : m_emptyVarDecls)
		for (solidity::assembly::TypedName const& varName : varDecl->variables)
			if (varName.name == _identifier)
				return *varDecl;

	yulAssert(false, "Unexpectly not found.");
}

template<typename StmtT>
std::vector<Statement>::iterator VarDeclPropagator::iteratorOf(StmtT& _stmt)
{
	auto it = find_if(
		begin(currentBlock().statements),
		end(currentBlock().statements),
		[&](Statement& checkStmt) { return checkStmt.type() == typeid(StmtT)
											&& &boost::get<StmtT>(checkStmt) == &_stmt; }
	);
	yulAssert(it != end(currentBlock().statements), "");
	return it;
}

TypedNameList VarDeclPropagator::recreateTypedNameList(
	std::vector<Identifier> const& orderedIdentifiers,
	TypedNameList const& typeHints) const
{
	TypedNameList aux;
	aux.reserve(orderedIdentifiers.size());

	for (Identifier const& ident : orderedIdentifiers)
	{
		TypedName const& hint = findTypedName(typeHints, ident);
		aux.emplace_back(TypedName{hint.location, ident.name, hint.type});
	}

	return aux;
}

void VarDeclPropagator::removeVarDecl(VariableDeclaration const& varDecl)
{
	auto varDeclIterator = find_if(
		begin(currentBlock().statements), 
		end(currentBlock().statements),
		[&](Statement& _stmt) { return _stmt.type() == typeid(VariableDeclaration)
									&& &boost::get<VariableDeclaration>(_stmt) == &varDecl; }
	);
	yulAssert(varDeclIterator != end(currentBlock().statements), "");
	currentBlock().statements.erase(varDeclIterator);
}

TypedName const& VarDeclPropagator::findTypedName(TypedNameList const& list, Identifier const& ident)
{
	for (TypedName const& typedName : list)
		if (typedName.name == ident.name)
			return typedName;

	yulAssert(false, "TypedName must be present in list.");
}

