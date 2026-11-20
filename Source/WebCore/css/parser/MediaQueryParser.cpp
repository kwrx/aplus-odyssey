// Copyright 2014 The Chromium Authors. All rights reserved.
// Copyright (C) 2016 Apple Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "config.h"
#include "MediaQueryParser.h"

#include "CSSTokenizer.h"
#include "MediaList.h"
#include "MediaQueryParserContext.h"
#include <wtf/Vector.h>

namespace WebCore {

RefPtr<MediaQuerySet> MediaQueryParser::parseMediaQuerySet(const String& queryString, MediaQueryParserContext context)
{
    auto tokenizer = CSSTokenizer::tryCreate(queryString);
    if (UNLIKELY(!tokenizer))
        return nullptr;
    return parseMediaQuerySet(tokenizer->tokenRange(), context);
}

RefPtr<MediaQuerySet> MediaQueryParser::parseMediaQuerySet(CSSParserTokenRange range, MediaQueryParserContext context)
{
    return MediaQueryParser(MediaQuerySetParser, context).parseInternal(range);
}

RefPtr<MediaQuerySet> MediaQueryParser::parseMediaCondition(CSSParserTokenRange range, MediaQueryParserContext context)
{
    return MediaQueryParser(MediaConditionParser, context).parseInternal(range);
}

const MediaQueryParser::State MediaQueryParser::ReadRestrictor = &MediaQueryParser::readRestrictor;
const MediaQueryParser::State MediaQueryParser::ReadMediaNot = &MediaQueryParser::readMediaNot;
const MediaQueryParser::State MediaQueryParser::ReadMediaType = &MediaQueryParser::readMediaType;
const MediaQueryParser::State MediaQueryParser::ReadAnd = &MediaQueryParser::readAnd;
const MediaQueryParser::State MediaQueryParser::ReadFeatureStart = &MediaQueryParser::readFeatureStart;
const MediaQueryParser::State MediaQueryParser::ReadFeature = &MediaQueryParser::readFeature;
const MediaQueryParser::State MediaQueryParser::ReadFeatureColon = &MediaQueryParser::readFeatureColon;
const MediaQueryParser::State MediaQueryParser::ReadFeatureValue = &MediaQueryParser::readFeatureValue;
const MediaQueryParser::State MediaQueryParser::ReadFeatureEnd = &MediaQueryParser::readFeatureEnd;
const MediaQueryParser::State MediaQueryParser::SkipUntilComma = &MediaQueryParser::skipUntilComma;
const MediaQueryParser::State MediaQueryParser::SkipUntilBlockEnd = &MediaQueryParser::skipUntilBlockEnd;
const MediaQueryParser::State MediaQueryParser::Done = &MediaQueryParser::done;

MediaQueryParser::MediaQueryParser(ParserType parserType, MediaQueryParserContext context)
    : m_parserType(parserType)
    , m_mediaQueryData(context)
    , m_querySet(MediaQuerySet::create())
    
{
    if (parserType == MediaQuerySetParser)
        m_state = &MediaQueryParser::readRestrictor;
    else // MediaConditionParser
        m_state = &MediaQueryParser::readMediaNot;
}

MediaQueryParser::~MediaQueryParser() = default;

void MediaQueryParser::setStateAndRestrict(State state, MediaQuery::Restrictor restrictor)
{
    m_mediaQueryData.setRestrictor(restrictor);
    m_state = state;
}

// State machine member functions start here
void MediaQueryParser::readRestrictor(CSSParserTokenType type, const CSSParserToken& token, CSSParserTokenRange& range)
{
    readMediaType(type, token, range);
}

void MediaQueryParser::readMediaNot(CSSParserTokenType type, const CSSParserToken& token, CSSParserTokenRange& range)
{
    if (type == IdentToken && equalIgnoringASCIICase(token.value(), "not"))
        setStateAndRestrict(ReadFeatureStart, MediaQuery::Not);
    else
        readFeatureStart(type, token, range);
}

static bool isRestrictorOrLogicalOperator(const CSSParserToken& token)
{
    // FIXME: it would be more efficient to use lower-case always for tokenValue.
    return equalIgnoringASCIICase(token.value(), "not")
        || equalIgnoringASCIICase(token.value(), "and")
        || equalIgnoringASCIICase(token.value(), "or")
        || equalIgnoringASCIICase(token.value(), "only");
}

void MediaQueryParser::readMediaType(CSSParserTokenType type, const CSSParserToken& token, CSSParserTokenRange& range)
{
    if (type == LeftParenthesisToken) {
        if (m_mediaQueryData.restrictor() != MediaQuery::None)
            m_state = SkipUntilComma;
        else
            m_state = ReadFeature;
    } else if (type == IdentToken) {
        if (m_state == ReadRestrictor && equalIgnoringASCIICase(token.value(), "not"))
            setStateAndRestrict(ReadMediaType, MediaQuery::Not);
        else if (m_state == ReadRestrictor && equalIgnoringASCIICase(token.value(), "only"))
            setStateAndRestrict(ReadMediaType, MediaQuery::Only);
        else if (m_mediaQueryData.restrictor() != MediaQuery::None
            && isRestrictorOrLogicalOperator(token)) {
            m_state = SkipUntilComma;
        } else {
            m_mediaQueryData.setMediaType(token.value().toString());
            m_state = ReadAnd;
        }
    } else if (type == EOFToken && (!m_querySet->queryVector().size() || m_state != ReadRestrictor))
        m_state = Done;
    else {
        m_state = SkipUntilComma;
        if (type == CommaToken)
            skipUntilComma(type, token, range);
    }
}

void MediaQueryParser::commitMediaQuery()
{
    // FIXME-NEWPARSER: Convoluted and awful, but we can't change the MediaQuerySet yet because of the
    // old parser.
    static const NeverDestroyed<String> defaultMediaType { "all"_s };
    MediaQuery mediaQuery { m_mediaQueryData.restrictor(), m_mediaQueryData.mediaType().value_or(defaultMediaType), WTFMove(m_mediaQueryData.expressions()) };
    m_mediaQueryData.clear();
    m_querySet->addMediaQuery(WTFMove(mediaQuery));
}

void MediaQueryParser::readAnd(CSSParserTokenType type, const CSSParserToken& token, CSSParserTokenRange& /*range*/)
{
    if (type == IdentToken && equalIgnoringASCIICase(token.value(), "and")) {
        m_state = ReadFeatureStart;
    } else if (type == CommaToken && m_parserType != MediaConditionParser) {
        commitMediaQuery();
        m_state = ReadRestrictor;
    } else if (type == EOFToken)
        m_state = Done;
    else
        m_state = SkipUntilComma;
}

void MediaQueryParser::readFeatureStart(CSSParserTokenType type, const CSSParserToken& /*token*/, CSSParserTokenRange& /*range*/)
{
    if (type == LeftParenthesisToken)
        m_state = ReadFeature;
    else
        m_state = SkipUntilComma;
}

enum class RangeOperator { Less, LessEqual, Greater, GreaterEqual, Equal };

static bool isRangeDelimiter(const CSSParserToken& token)
{
    return token.type() == DelimiterToken && (token.delimiter() == '<' || token.delimiter() == '>' || token.delimiter() == '=');
}

static RangeOperator flipRangeOperator(RangeOperator op)
{
    switch (op) {
    case RangeOperator::Less:
        return RangeOperator::Greater;
    case RangeOperator::LessEqual:
        return RangeOperator::GreaterEqual;
    case RangeOperator::Greater:
        return RangeOperator::Less;
    case RangeOperator::GreaterEqual:
        return RangeOperator::LessEqual;
    case RangeOperator::Equal:
        return RangeOperator::Equal;
    }
    return op;
}

bool MediaQueryParser::readRangeFeature(const CSSParserToken& first, CSSParserTokenRange& range)
{
    if (first.type() == EOFToken || first.getBlockType() == CSSParserToken::BlockEnd) {
        return false;
    }

    const CSSParserToken* begin = &first;
    const CSSParserToken* end = nullptr;
    bool hasComparison = isRangeDelimiter(first);
    unsigned depth = first.getBlockType() == CSSParserToken::BlockStart ? 1 : 0;

    CSSParserTokenRange scan = range;
    while (!scan.atEnd()) {
        const CSSParserToken& token = scan.peek();
        if (token.getBlockType() == CSSParserToken::BlockStart) {
            depth++;
        } else if (token.getBlockType() == CSSParserToken::BlockEnd) {
            if (!depth) {
                end = &token;
                break;
            }
            depth--;
        } else if (!depth && isRangeDelimiter(token)) {
            hasComparison = true;
        }
        scan.consume();
    }

    if (!end || !hasComparison) {
        return false;
    }

    Vector<std::pair<const CSSParserToken*, const CSSParserToken*>, 3> operands;
    Vector<RangeOperator, 2> operators;
    bool malformed = false;

    const CSSParserToken* operandStart = begin;
    depth = 0;
    for (const CSSParserToken* token = begin; token < end; ++token) {
        if (token->getBlockType() == CSSParserToken::BlockStart) {
            depth++;
            continue;
        }
        if (token->getBlockType() == CSSParserToken::BlockEnd) {
            depth--;
            continue;
        }
        if (depth || !isRangeDelimiter(*token)) {
            continue;
        }

        operands.append({ operandStart, token });

        UChar delimiter = token->delimiter();
        bool orEqual = false;
        if (delimiter != '=' && token + 1 < end && token[1].type() == DelimiterToken && token[1].delimiter() == '=') {
            orEqual = true;
            ++token;
        }

        if (delimiter == '<') {
            operators.append(orEqual ? RangeOperator::LessEqual : RangeOperator::Less);
        } else if (delimiter == '>') {
            operators.append(orEqual ? RangeOperator::GreaterEqual : RangeOperator::Greater);
        } else {
            operators.append(RangeOperator::Equal);
        }

        operandStart = token + 1;
    }
    operands.append({ operandStart, end });

    auto trimmed = [&](std::pair<const CSSParserToken*, const CSSParserToken*> operand) {
        while (operand.first < operand.second && operand.first->type() == WhitespaceToken) {
            ++operand.first;
        }
        while (operand.second > operand.first && operand.second[-1].type() == WhitespaceToken) {
            --operand.second;
        }
        return operand;
    };

    for (auto& operand : operands) {
        operand = trimmed(operand);
        if (operand.first == operand.second) {
            malformed = true;
        }
    }

    auto isFeatureName = [](std::pair<const CSSParserToken*, const CSSParserToken*> operand) {
        return operand.second - operand.first == 1 && operand.first->type() == IdentToken;
    };

    Vector<MediaQueryExpression, 2> expressions;

    auto build = [&](const String& feature, RangeOperator op, std::pair<const CSSParserToken*, const CSSParserToken*> operand) {
        String name = feature;
        if (op == RangeOperator::Greater || op == RangeOperator::GreaterEqual) {
            name = makeString("min-", feature);
        } else if (op == RangeOperator::Less || op == RangeOperator::LessEqual) {
            name = makeString("max-", feature);
        }

        CSSParserTokenRange valueRange = range.makeSubRange(operand.first, operand.second);
        MediaQueryExpression expression { name, valueRange, m_mediaQueryData.context() };
        valueRange.consumeWhitespace();
        if (!valueRange.atEnd() || !expression.value()) {
            CSSParserTokenRange empty = range.makeSubRange(operand.first, operand.first);
            expressions.append(MediaQueryExpression { emptyString(), empty, m_mediaQueryData.context() });
            return;
        }

        if (op == RangeOperator::Greater || op == RangeOperator::Less) {
            expression.makeStrict(op == RangeOperator::Greater);
        }
        expressions.append(WTFMove(expression));
    };

    if (malformed) {
        operands.clear();
    }

    if (operands.size() == 2 && isFeatureName(operands[0]) && !isFeatureName(operands[1])) {
        build(operands[0].first->value().toString(), operators[0], operands[1]);
    } else if (operands.size() == 2 && isFeatureName(operands[1]) && !isFeatureName(operands[0])) {
        build(operands[1].first->value().toString(), flipRangeOperator(operators[0]), operands[0]);
    } else if (operands.size() == 3 && isFeatureName(operands[1]) && !isFeatureName(operands[0]) && !isFeatureName(operands[2])
        && operators[0] != RangeOperator::Equal && operators[1] != RangeOperator::Equal
        && ((operators[0] == RangeOperator::Less || operators[0] == RangeOperator::LessEqual) == (operators[1] == RangeOperator::Less || operators[1] == RangeOperator::LessEqual))) {
        String feature = operands[1].first->value().toString();
        build(feature, flipRangeOperator(operators[0]), operands[0]);
        build(feature, operators[1], operands[2]);
    }

    bool valid = !expressions.isEmpty();
    for (auto& expression : expressions) {
        if (!expression.isValid()) {
            valid = false;
        }
    }

    if (valid) {
        for (auto& expression : expressions) {
            m_mediaQueryData.appendExpression(WTFMove(expression));
        }
    } else {
        CSSParserTokenRange empty = range.makeSubRange(end, end);
        m_mediaQueryData.appendExpression(MediaQueryExpression { emptyString(), empty, m_mediaQueryData.context() });
    }

    while (!range.atEnd() && &range.peek() != end) {
        m_blockWatcher.handleToken(range.peek());
        range.consume();
    }

    m_state = ReadFeatureEnd;
    return true;
}

void MediaQueryParser::readFeature(CSSParserTokenType type, const CSSParserToken& token, CSSParserTokenRange& range)
{
    if (readRangeFeature(token, range)) {
        return;
    }

    if (type == IdentToken) {
        m_mediaQueryData.setMediaFeature(token.value().toString());
        m_state = ReadFeatureColon;
    } else
        m_state = SkipUntilComma;
}

void MediaQueryParser::readFeatureColon(CSSParserTokenType type, const CSSParserToken& token, CSSParserTokenRange& range)
{
    if (type == ColonToken) {
        while (range.peek().type() == WhitespaceToken)
            range.consume();
        if (range.peek().type() == RightParenthesisToken || range.peek().type() == EOFToken)
            m_state = SkipUntilBlockEnd;
        else
            m_state = ReadFeatureValue;
    } else if (type == RightParenthesisToken || type == EOFToken) {
        m_mediaQueryData.addExpression(range);
        readFeatureEnd(type, token, range);
    } else
        m_state = SkipUntilBlockEnd;
}

void MediaQueryParser::readFeatureValue(CSSParserTokenType type, const CSSParserToken& token, CSSParserTokenRange& range)
{
    if (type == DimensionToken && token.unitType() == CSSUnitType::CSS_UNKNOWN) {
        range.consume();
        m_state = SkipUntilComma;
    } else {
        m_mediaQueryData.addExpression(range);
        m_state = ReadFeatureEnd;
    }
}

void MediaQueryParser::readFeatureEnd(CSSParserTokenType type, const CSSParserToken& /*token*/, CSSParserTokenRange& /*range*/)
{
    if (type == RightParenthesisToken || type == EOFToken) {
        if (type != EOFToken && m_mediaQueryData.lastExpressionValid())
            m_state = ReadAnd;
        else
            m_state = SkipUntilComma;
    } else {
        m_mediaQueryData.removeLastExpression();
        m_state = SkipUntilBlockEnd;
    }
}

void MediaQueryParser::skipUntilComma(CSSParserTokenType type, const CSSParserToken& /*token*/, CSSParserTokenRange& /*range*/)
{
    if ((type == CommaToken && !m_blockWatcher.blockLevel()) || type == EOFToken) {
        m_state = ReadRestrictor;
        m_mediaQueryData.clear();
        MediaQuery query = MediaQuery(MediaQuery::Not, "all", Vector<MediaQueryExpression>());
        m_querySet->addMediaQuery(WTFMove(query));
    }
}

void MediaQueryParser::skipUntilBlockEnd(CSSParserTokenType /*type */, const CSSParserToken& token, CSSParserTokenRange& /*range*/)
{
    if (token.getBlockType() == CSSParserToken::BlockEnd && !m_blockWatcher.blockLevel())
        m_state = SkipUntilComma;
}

void MediaQueryParser::done(CSSParserTokenType /*type*/, const CSSParserToken& /*token*/, CSSParserTokenRange& /*range*/) { }

void MediaQueryParser::handleBlocks(const CSSParserToken& token)
{
    if (token.getBlockType() == CSSParserToken::BlockStart
        && (token.type() != LeftParenthesisToken || m_blockWatcher.blockLevel()))
            m_state = SkipUntilBlockEnd;
}

void MediaQueryParser::processToken(const CSSParserToken& token, CSSParserTokenRange& range)
{
    CSSParserTokenType type = token.type();

    if (m_state != ReadFeatureValue || type == WhitespaceToken) {
        handleBlocks(token);
        m_blockWatcher.handleToken(token);
        range.consume();
    }

    // Call the function that handles current state
    if (type != WhitespaceToken)
        ((this)->*(m_state))(type, token, range);
}

// The state machine loop
RefPtr<MediaQuerySet> MediaQueryParser::parseInternal(CSSParserTokenRange range)
{
    while (!range.atEnd())
        processToken(range.peek(), range);

    // FIXME: Can we get rid of this special case?
    if (m_parserType == MediaQuerySetParser)
        processToken(CSSParserToken(EOFToken), range);

    if (m_state != ReadAnd && m_state != ReadRestrictor && m_state != Done && m_state != ReadMediaNot) {
        MediaQuery query = MediaQuery(MediaQuery::Not, "all", Vector<MediaQueryExpression>());
        m_querySet->addMediaQuery(WTFMove(query));
    } else if (m_mediaQueryData.currentMediaQueryChanged())
        commitMediaQuery();

    m_querySet->shrinkToFit();

    return m_querySet;
}

MediaQueryParser::MediaQueryData::MediaQueryData(MediaQueryParserContext context)
    : m_context(context)
{
}

void MediaQueryParser::MediaQueryData::clear()
{
    m_restrictor = MediaQuery::None;
    m_mediaType = std::nullopt;
    m_mediaFeature = String();
    m_expressions.clear();
}

void MediaQueryParser::MediaQueryData::addExpression(CSSParserTokenRange& range)
{
    m_expressions.append(MediaQueryExpression { m_mediaFeature, range, m_context });
}

bool MediaQueryParser::MediaQueryData::lastExpressionValid()
{
    return m_expressions.last().isValid();
}

void MediaQueryParser::MediaQueryData::removeLastExpression()
{
    m_expressions.removeLast();
}

} // namespace WebCore
