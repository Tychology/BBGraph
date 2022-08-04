#pragma once

#include <JuceHeader.h>

struct GlobalValues
{
	double fs;
	double f;
	double ps;
	double p;
	double rs;
	double r;
	double n;
	double t;

	double nf;
	double sr;
	double bps;
};

class ByteCodeProcessor
{
	enum Op
	{
		invert,
		add,
		subtract,
		multiply,
		divide,
		modulo,

		bitnot,
		bitand,
		bitor ,
		bitxor,
		lshift,
		rshift,

		not,
		and,
		or ,

		equal,
		notequal,
		less,
		lessorequal,
		greater,
		greaterorequal,

		power,

		sqrt,
		cbrt,

		exp,
		exp2,
		log,
		log2,
		log10,

		absolute,

		sine,
		cosine,
		tangent,
		arcsine,
		arccosine,
		arctangent,

		numberConstant,

		pi,
		twopi,
		halfpi,
		e,

		random,

		fs,
		f,
		ps,
		p,
		rs,
		r,
		n,
		t,

		nf,
		sr,
		bps,

		a,
		b,
		c,
		d,

		lparenthesis,
		rparenthesis,

		error,
	};

	enum Assoc { none = 0, left, right };

	struct TokenProperties
	{
		const char* str;
		const Op op;
		const int prec;
		const Assoc assoc;
		const int arity;
	};

	static constexpr TokenProperties tokens[]
	{
		{"_", invert, 2, right, 1}, //using "_" for the unary minus to make parsing easier
		{"+", add, 6, left, 2},
		{"-", subtract, 6, left, 2},
		{"*", multiply, 5, left, 2},
		{"/", divide, 5, left, 2},
		{"%", modulo, 5, left, 2},

		{"~", bitnot, 2, right, 1},
		{"&", bitand, 11, left, 2},
		{"|", bitor , 13, left, 2},
		{"^", bitxor, 12, left, 2},
		{"<<", lshift, 7, left, 2},
		{">>", rshift, 7, left, 2},

		{"!", not, 2, right, 1},
		{"&&",and, 14, left, 2},
		{"||", or , 15, left, 2},

		{"==", equal, 10, left, 2},
		{"!=", notequal, 10, left, 2},
		{"<", less, 9, left, 2},
		{"<=", lessorequal, 9, left, 2},
		{">", greater, 9, left, 2},
		{">=", greaterorequal, 9, left, 2},

		{"**", power, 3, right, 2},

		{"sqrt", sqrt, 2, right, 1},
		{"cbrt", cbrt, 2, right, 1},

		{"exp", exp, 2, right, 1},
		{"exp2", exp2, 2, right, 1},
		{"log", log, 2, right, 1},
		{"log2", log2, 2, right, 1},
		{"log10", log10, 2, right, 1},


		{"abs", absolute, 2, right, 1},


		{"sin", sine, 2, right, 1},
		{"cos", cosine, 2, right, 1},
		{"tan", tangent, 2, right, 1},
		{"asin", arcsine, 2, right, 1},
		{"acos", arccosine, 2, right, 1},
		{"atan", arctangent, 2, right, 1},


		{"", numberConstant, 0, none, 0},

		{"pi", pi, 0, none, 0},
		{"twoPi", twopi, 0, none, 0},
		{"halfPi", halfpi, 0, none, 0},
		{"e", e, 0, none, 0},

		{"rand", random, 0, none, 0},

		{"fs", fs, 0, none, 0},
		{"f", f, 0, none, 0},
		{"ps", ps, 0, none, 0},
		{"p", p, 0, none, 0},
		{"rs", rs, 0, none, 0},
		{"r", r, 0, none, 0},
		{"n", n, 0, none, 0},
		{"t", t, 0, none, 0},

		{"nf", nf, 0, none, 0},
		{"sr", sr, 0, none, 0},
		{"bps", bps, 0, none, 0},


		{"a", a, 0, none, 0},
		{"b", b, 0, none, 0},
		{"c", c, 0, none, 0},
		{"d", d, 0, none, 0},


		{"(", lparenthesis, 0, none, -1},
		{")", rparenthesis, 0, none, -1}


	};

	enum State { newToken, minusRead, readNumber, readWord, readSymbols };

public:
	bool update(juce::StringRef exprStr);

	double process(const double* inputValues, const GlobalValues globalValues);
	
private:
	static Op getTokenFromString(std::string const& buffer);

	static bool tokenize(juce::StringRef expressionString, std::vector<Op>& tokenSequence,
		std::vector<double>& numberConstants);

	static int parsePostfix(const std::vector<Op>& tokenSequence);

	bool infixToPostfix(std::vector<Op>& tokenSequence) const;

	std::vector<Op> byteCode;
	std::vector<double> numberConstants;
	std::vector<double> processingStack;
};
