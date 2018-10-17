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

#pragma once

#include <libyul/optimiser/ASTWalker.h>
#include <libyul/Exceptions.h>
#include <libsolidity/inlineasm/AsmData.h>
#include <list>
#include <stack>
#include <vector>

namespace dev
{
namespace yul
{

/*
 * Requirements:
 * - Disambiguration pass
 */
class VarDeclPropagator: public ASTModifier
{
public:
	using ASTModifier::operator();
	void operator()(Block& _block) override;
	void operator()(VariableDeclaration& _varDecl) override;
	void operator()(Assignment& _assignment) override;

private:
	using TypedNameList = solidity::assembly::TypedNameList;
	using TypedName = solidity::assembly::TypedName;
	using Statement = solidity::assembly::Statement;

	inline Block& currentBlock()
	{
		yulAssert(!m_blockScopes.empty(), "Called outside block.");
		return *m_blockScopes.top();
	}

	bool checkAllIdentifiersMatchSame(std::vector<Identifier> const& identifiers) const;
	VariableDeclaration const& getVarDecl(std::string const& varName) const noexcept;
	void removeVarDecl(VariableDeclaration const& varDecl);
	TypedNameList recreateTypedNameList(std::vector<Identifier> const& orderedIdentifiers, TypedNameList const& typeHints) const;

	template<typename StmtT> std::vector<Statement>::iterator iteratorOf(StmtT& stmt);

	static TypedName const& findTypedName(TypedNameList const& list, Identifier const& ident);

private:
	std::list<VariableDeclaration*> m_emptyVarDecls;
	std::list<Assignment*> m_assignments; // all assignments to empty var-decls
	std::stack<Block*> m_blockScopes;
};

}
}
