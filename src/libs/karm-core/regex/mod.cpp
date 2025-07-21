module;

#include <karm-base/flags.h>
#include <karm-base/rc.h>
#include <karm-io/expr.h>
#include <karm-io/sscan.h>

export module Karm.Core:regex;

namespace Karm::Regex {

enum struct Option {
    GLOBAL = 1 << 0,      // All matches (don't return after first match)
    MULTI_LINE = 1 << 1,  // ^ and $ match start/end of line
    INSENSITIVE = 1 << 2, // Case-insensitive match
    STICKY = 1 << 3,      // Force the pattern to only match consecutive matches from where the previous match ended.
    UNICODE = 1 << 4,     // Match with full Unicode
    VNICODE = 1 << 5,
    SINGLE_LINE = 1 << 6, // Dot matches newline characters
};

enum struct Flavor {
    ECMA_SCRIPT
};

// MARK: Matcher ---------------------------------------------------------------

struct Match {
    urange range;
    Str data;
};

struct Matcher {
    virtual ~Matcher() = default;

    virtual Opt<Match> match();
};

struct InfixMatcher : Matcher {
    enum struct Type {
        EITHER,
        CHAIN
    };

    using enum Type;

    Type _type;
    Rc<Matcher> _lhs;
    Rc<Matcher> _rhs;

    InfixMatcher(Type type, Rc<Matcher> lhs, Rc<Matcher> rhs)
        : _type(type), _lhs(lhs), _rhs(rhs) {}
};

struct NopMatcher : Matcher {
};

struct AtomMatcher : Matcher {
    String data;
};

struct Quantifier {
    u64 min;
    Opt<u64> max;
};

// MARK: Parser ----------------------------------------------------------------

// https://tc39.es/ecma262/#prod-SyntaxCharacter
static auto RE_SYNTAX_CHARACTER = Re::single('^', '$', '\\', '.', '*', '+', '?', '(', ')', '[', ']', '{', '}', '|');

// https://tc39.es/ecma262/#prod-PatternCharacter
static auto RE_PATTERN_CHARACTER = Re::negate(RE_SYNTAX_CHARACTER);

static Res<Quantifier> _parseQuantifier(Io::SScan& s) {
    if (s.skip("*"))
        return Ok(Quantifier{0, NONE});
    else if (s.skip("+"))
        return Ok(Quantifier{1, NONE});
    else if (s.skip("?"))
        return Ok(Quantifier{0, 1});
    else {
        return Error::invalidData("expected quantifier");
    }
}

static Res<Rc<Matcher>> _parseAtom(Io::SScan& s) {
    if (auto chr = s.token(RE_PATTERN_CHARACTER)) {
        return Ok(makeRc<AtomMatcher>(chr));
    } else if (s.skip(".")) {

    } else if (s.ahead("\\")) {

    } else if (s.ahead("[")) {

    } else {
        return Error::invalidData("expected atom");
    }
}

static Res<Rc<Matcher>> _parseAssertion(Io::SScan& s) {}

static Res<Rc<Matcher>> _parseTerm(Io::SScan& s) {
    if (auto assert = _parseAssertion(s)) {
        return Ok(assert.take());
    } else if (auto atom = _parseAtom(s)) {
        return Ok(atom.take());
    } else {
        return Error::invalidData("expected term");
    }
}

static Res<Rc<Matcher>> _parseAlternative(Io::SScan& s) {
    auto lhs = _parseTerm(s).ok();

    if (not lhs)
        return Ok(makeRc<NopMatcher>());

    while (auto rhs = _parseTerm(s).ok())
        lhs = makeRc<InfixMatcher>(InfixMatcher::CHAIN, lhs.take(), rhs.take());

    return Ok(lhs.take());
}

static Res<Rc<Matcher>> _parseDisjunction(Io::SScan& s) {
    auto lhs = try$(_parseAlternative(s));
    if (not s.skip('|'))
        return Ok(lhs);
    auto rhs = try$(_parseDisjunction(s));
    return Ok(makeRc<InfixMatcher>(InfixMatcher::EITHER, lhs, rhs));
}

static Res<Rc<Matcher>> _parsePattern(Io::SScan& s) {
    return _parseDisjunction(s);
}

// MARK: Regex -----------------------------------------------------------------

export struct Regex {
    Rc<Matcher> _matcher;

    static Regex from(Str str, Flags<Option> options = {}, Flavor flavor = Flavor::ECMA_SCRIPT) {
        if (flavor != Flavor::ECMA_SCRIPT)
            panic("only ecma script flavored regex supported");
        Io::SScan s = str;
        return {
            _parsePattern(s)
                .unwrap("invalid regex pattern")
        };
    }

    Opt<Match> match(Str) {
    }
};

} // namespace Karm::Regex

export constexpr Karm::Regex::Regex operator""_regex(char const* str, usize len) {
    return Karm::Regex::Regex::from(Str{str, len});
}
