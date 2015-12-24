/******************************************************************************
 * Copyright (c) 2014-2015 Leandro T. C. Melo (ltcmelo@gmail.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 *****************************************************************************/

/*--------------------------*/
/*--- The UaiSo! Project ---*/
/*--------------------------*/

#include "Semantic/CompletionProposer.h"
#include "Semantic/Environment.h"
#include "Semantic/Program.h"
#include "Semantic/Symbol.h"
#include "Semantic/Type.h"
#include "Ast/Ast.h"
#include "Ast/AstVisitor.h"
#include "Common/Assert.h"
#include "Common/Trace__.h"
#include "Parsing/Factory.h"
#include "Parsing/Lexeme.h"
#include "Parsing/LexemeMap.h"
#include "Parsing/Syntax.h"
#include <iostream>
#include <stack>
#include <vector>

#define ENSURE_ANNOTATED_SYMBOL \
    UAISO_ASSERT(ast->sym_, return Abort, "no symbol annotated")
#define ENSURE_NONEMPTY_AST_STACK \
    UAISO_ASSERT(!asts_.empty(), return Abort)

#define TRACE_NAME "Completion"

using namespace uaiso;

namespace {

class CompletionContext final : public AstVisitor<CompletionContext>
{
public:
    using Base = AstVisitor<CompletionContext>;

    CompletionContext(const Syntax* syntax)
        : syntax_(syntax)
    {}

    bool analyse(const ProgramAst* progAst,
                 const LexemeMap* lexemes,
                 Environment env)
    {
        UAISO_ASSERT(lexemes, return false);

        lexemes_ = lexemes;
        env_ = env;

        auto result = traverseProgram(progAst, this, syntax_);

        return result == Abort;
    }

    Environment env_;
    const LexemeMap* lexemes_ { nullptr };
    std::stack<Ast*> asts_;
    size_t collectName_ { 0 }; // Collect names only when it matters.
    std::vector<const Ident*> name_;
    const Syntax* syntax_ { nullptr };

    VisitResult visitCompletionName(CompletionNameAst*)
    {
        // Visit only until completion name is found.
        return Abort;
    }

    VisitResult visitSimpleName(SimpleNameAst* ast)
    {
        if (collectName_) {
            auto name = lexemes_->find<Ident>(ast->nameLoc_.fileName_,
                                              ast->nameLoc_.lineCol());
            name_.push_back(name);
        }
        return Continue;
    }

    //--- Expressions ---//

    VisitResult traverseIdentExpr(IdentExprAst* ast)
    {
        asts_.push(ast);
        VIS_CALL(Base::traverseIdentExpr(ast));
        ENSURE_NONEMPTY_AST_STACK;
        asts_.pop();

        return Continue;
    }

    VisitResult traverseMemberAccessExpr(MemberAccessExprAst* ast)
    {
        asts_.push(ast);

        if (!collectName_)
            name_.clear();
        ++collectName_;
        if (ast->isExpr())
            VIS_CALL(traverseExpr(Expr_Cast(ast->exprOrSpec_.get())));
        else
            VIS_CALL(traverseSpec(Spec_Cast(ast->exprOrSpec_.get())));
        UAISO_ASSERT(collectName_ > 0, return Abort);
        --collectName_;

        VIS_CALL(traverseName(ast->name_.get()));
        ENSURE_NONEMPTY_AST_STACK;
        asts_.pop();

        return Continue;
    }

    //--- Specifiers ---//

    VisitResult traverseNamedSpec(NamedSpecAst* ast)
    {
        asts_.push(ast);

        if (!collectName_)
            name_.clear();
        ++collectName_;
        VIS_CALL(Base::traverseNamedSpec(ast));
        UAISO_ASSERT(collectName_ > 0, return Abort);
        --collectName_;

        ENSURE_NONEMPTY_AST_STACK;
        asts_.pop();

        return Continue;
    }

    // Track the current environment.

    VisitResult traverseRecordDecl(RecordDeclAst* ast)
    {
        ENSURE_ANNOTATED_SYMBOL;
        env_ = ast->sym_->type()->env();
        VIS_CALL(Base::traverseRecordDecl(ast));
        env_ = env_.outerEnv();
        return Continue;
    }

    VisitResult traverseFuncDecl(FuncDeclAst* ast)
    {
        if (syntax_->hasFuncLevelScope()) {
            ENSURE_ANNOTATED_SYMBOL;
            env_ = ast->sym_->env();
            VIS_CALL(Base::traverseFuncDecl(ast));
            env_ = env_.outerEnv();
        } else {
            VIS_CALL(Base::traverseFuncDecl(ast));
        }
        return Continue;
    }

    VisitResult traverseBlockStmt(BlockStmtAst* ast)
    {
        // The environment of a block stmt might be the shared with its
        // parent. If that's the case, it's been entered already.
        if (env_ == ast->env_
                || !syntax_->hasBlockLevelScope()) {
            VIS_CALL(Base::traverseBlockStmt(ast));
        } else {
            env_ = ast->env_;
            VIS_CALL(Base::traverseBlockStmt(ast));
            env_ = env_.outerEnv();
        }
        return Continue;
    }
};

} // anonymous

struct uaiso::CompletionProposer::CompletionProposerImpl
{
    CompletionProposerImpl(Factory* factory)
        : syntax_(factory->makeSyntax())
    {}

    //! Language-specific syntax.
    std::unique_ptr<const Syntax> syntax_;
};

CompletionProposer::CompletionProposer(Factory *factory)
    : P(new CompletionProposerImpl(factory))
{}

CompletionProposer::~CompletionProposer()
{}

std::pair<std::vector<const DeclSymbol*>, CompletionProposer::ResultCode>
CompletionProposer::propose(ProgramAst* progAst, const LexemeMap* lexemes)
{
    using Symbols = std::vector<const DeclSymbol*>;

    UAISO_ASSERT(progAst->program_,
                 return std::make_pair(Symbols(), CompletionAstNotFound));

    CompletionContext context(P->syntax_.get());
    auto ok = context.analyse(progAst, lexemes, progAst->program_->env());

    if (!ok) {
        DEBUG_TRACE("CompletionAstNotFound\n");
        return std::make_pair(Symbols(), CompletionAstNotFound);
    }

    if (context.asts_.empty()) {
        DEBUG_TRACE("UnboundCompletionAstContext\n");
        return std::make_pair(Symbols() , UnboundCompletionAstContext);
    }

    auto topAst = context.asts_.top();
    auto env = context.env_;

    // When completing an identifier, simply list the environment.
    if (topAst->kind() == Ast::Kind::IdentExpr) {
        Symbols syms;
        while (true) {
            auto curSyms = env.list();
            std::copy(curSyms.begin(), curSyms.end(), std::back_inserter(syms));
            if (env.isRootEnv())
                break;
            env = env.outerEnv();
        }
        return std::make_pair(syms, ResultOK);
    }

    // Completing on a member access requires identifying the type or namespace.
    if (topAst->kind() == Ast::Kind::MemberAccessExpr
            || topAst->kind() == Ast::Kind::NamedSpec) {
        const Type* ty = nullptr;
        for (auto ident : context.name_) {
            auto tySym = env.lookUpType(ident);
            if (tySym) {
                ty = tySym->type();
            } else {
                auto valSym = env.lookUpValue(ident);
                if (valSym) {
                    ty = valSym->valueType();
                } else {
                    auto spaceSym = env.fetchNamespace(ident);
                    if (!spaceSym) {
                        DEBUG_TRACE("UnknownSymbol\n");
                        return std::make_pair(Symbols(), UnknownSymbol);
                    }

                    // Enter the namespace and start it over.
                    env = spaceSym->env();
                    continue;
                }
            }

            if (!ty)
                return std::make_pair(Symbols(), UndefinedType);

            // TODO: Extract a type resolver out of code below.

            // If this is an elaborate type, try resolving to the actual type.
            if (ty->kind() == Type::Kind::Elaborate) {
                auto elabTy = ElaborateType_ConstCast(ty);
                if (!elabTy->isResolved()) {
                    tySym = env.lookUpType(elabTy->name());
                    if (!tySym) {
                        DEBUG_TRACE("UnresolvedElaborateType\n");
                        return std::make_pair(Symbols(), UnresolvedElaborateType);
                    }

                    ty = tySym->type();
                    if (!ty) {
                        DEBUG_TRACE("UndefinedType\n");
                        return std::make_pair(Symbols(), UndefinedType);
                    }
                }
            }

            // In this case, trying again after type-checking might work.
            if (ty->kind() == Type::Kind::Inferred) {
                DEBUG_TRACE("TypeNotYetInferred\n");
                return std::make_pair(Symbols(), TypeNotYetInferred);
            }

            bool hasEnv;
            std::tie(hasEnv, env) = isEnvType(ty);
            if (!hasEnv) {
                DEBUG_TRACE("InvalidType\n");
                return std::make_pair(Symbols(), InvalidType);
            }
        }

        if (!ty)
            return std::make_pair(env.list(), ResultOK);

        // Look into base classes.
        Symbols syms = env.list();
        std::stack<const Type*> baseTys;
        baseTys.push(ty);
        while (!baseTys.empty()) {
            const Type* curTy = baseTys.top();
            baseTys.pop();
            UAISO_ASSERT(curTy, return std::make_pair(Symbols(), InternalError));
            if (curTy->kind() != Type::Kind::Record)
                continue;

            const RecordType* recTy = ConstRecordType_Cast(curTy);
            auto bases = recTy->bases();
            for (auto base : bases) {
                auto baseSym = env.lookUpType(base->name());
                if (!baseSym)
                    continue;

                bool baseHasEnv;
                Environment baseEnv;
                std::tie(baseHasEnv, baseEnv) = isEnvType(baseSym->type());
                if (baseHasEnv) {
                    baseTys.push(baseSym->type());
                    auto extraSyms = baseEnv.list();
                    std::copy(extraSyms.begin(), extraSyms.end(), std::back_inserter(syms));
                }
            }
        }

        return std::make_pair(syms, ResultOK);
    }

    DEBUG_TRACE("CaseNotImplemented\n");
    return std::make_pair(Symbols(), CaseNotImplemented);
}
