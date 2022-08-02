/*
  ==============================================================================

    ByteCodeProcessor.h
    Created: 25 Apr 2022 6:31:56pm
    Author:  Jonas

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

struct CounterValues
{
	double t = 0.f;
	double h = 0.f;
	double n = 0.f;
	double bpm = 0.f;
};


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
		bitor,
		bitxor,
		lshift,
		rshift,

		not,
		and,
		or,

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

	enum Assoc { none=0, left, right };

	struct tokenProperties
	{
		const char* str;
		const Op op;
		const int prec;
		const Assoc assoc;
		const int arity;
	};

	static constexpr tokenProperties tokens[]
	{

		{"_", invert, 2, right, 1}, //using "_" for the unary minus to make parsing easier
		{"+", add, 6, left, 2},
		{"-", subtract, 6, left, 2},
		{"*", multiply, 5, left, 2},
		{"/", divide, 5, left, 2},
		{"%", modulo, 5, left, 2},

		{"~", bitnot, 2, right, 1},
		{"&", bitand, 11, left, 2},
		{"|", bitor, 13, left, 2},
		{"^", bitxor, 12, left, 2},
		{"<<", lshift, 7, left, 2},
		{">>", rshift, 7, left, 2},

		{"!", not, 2, right, 1},
		{"&&", and, 14, left, 2},
		{"||", or, 15, left, 2},

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
	bool update(juce::StringRef exprStr) //, juce::String& errorStr)
	{
		std::vector<Op> tokenSequence;
		std::vector<double> nums;
		int maxStackSize = 0;

		if (!tokenize(exprStr, tokenSequence, nums)) return false;

		//Try to parse as postfix. If it fails, try to convert from infix to postfix, then try to parse as postfix again

		maxStackSize = parsePostfix(tokenSequence);
		if (maxStackSize == 0)
		{
			if (!infixToPostfix(tokenSequence)) return false;
			
			maxStackSize = parsePostfix(tokenSequence);
			if (maxStackSize == 0) return false;
		}


		processingStack.resize(maxStackSize);

		std::swap(byteCode, tokenSequence); //Make threadsafe
		std::swap(numberConstants, nums);

		return true;
	}


	double process(const double* inputValues, const GlobalValues globalValues) //const CounterValues& counterValues)
	{
		if (byteCode.empty()) return 0;

		int top = -1;

		auto* codePtr = byteCode.data();
		auto* numPtr = numberConstants.data();
		auto* stackPtr = processingStack.data();

		int nextNum = 0;


		for (auto op : byteCode)
		{
			switch (op)
			{
			case invert: stackPtr[top] = -stackPtr[top];
				break;
			case add: jassert(top >= 1);
				stackPtr[top - 1] = stackPtr[top - 1] + stackPtr[top];
				--top;
				break;
			case subtract: stackPtr[top - 1] = stackPtr[top - 1] - stackPtr[top];
				--top;
				break;
			case multiply: stackPtr[top - 1] = stackPtr[top - 1] * stackPtr[top];
				--top;
				break;
			case divide: stackPtr[top - 1] = stackPtr[top - 1] / stackPtr[top];
				--top;
				break;

			//case modulo:  if ((int)stackPtr[top]) stackPtr[top - 1] = (int)stackPtr[top - 1] % (int)stackPtr[top]; else return 0; --top; break;
			case modulo: stackPtr[top - 1] = std::fmod(stackPtr[top - 1], stackPtr[top]);
				--top;
				break;

			case bitnot: stackPtr[top] = ~(int)stackPtr[top];
				break;
			case bitand: stackPtr[top - 1] = (int)stackPtr[top - 1] & (int)stackPtr[top];
				--top;
				break;
			case bitor: stackPtr[top - 1] = (int)stackPtr[top - 1] | (int)stackPtr[top];
				--top;
				break;
			case bitxor: stackPtr[top - 1] = (int)stackPtr[top - 1] ^ (int)stackPtr[top];
				--top;
				break;
			case lshift: stackPtr[top - 1] = (long)stackPtr[top - 1] << (int)stackPtr[top];
				--top;
				break;
			case rshift: stackPtr[top - 1] = (int)stackPtr[top - 1] >> (int)stackPtr[top];
				--top;
				break;

			case not: stackPtr[top] = !(int)stackPtr[top];
				break;
			case and: stackPtr[top - 1] = (int)stackPtr[top - 1] && (int)stackPtr[top];
				--top;
				break;
			case or: stackPtr[top - 1] = (int)stackPtr[top - 1] || (int)stackPtr[top];
				--top;
				break;

			case equal: stackPtr[top - 1] = juce::approximatelyEqual(stackPtr[top - 1], stackPtr[top]);
				--top;
				break;
			case notequal: stackPtr[top - 1] = !juce::approximatelyEqual(stackPtr[top - 1], stackPtr[top]);
				--top;
				break;
			case less: stackPtr[top - 1] = stackPtr[top - 1] < stackPtr[top];
				--top;
				break;
			case lessorequal: stackPtr[top - 1] = stackPtr[top - 1] < stackPtr[top] || juce::approximatelyEqual(stackPtr[top - 1], stackPtr[top]);
				--top;
				break;
			case greater: stackPtr[top - 1] = stackPtr[top - 1] > stackPtr[top];
				--top;
				break;
			case greaterorequal: stackPtr[top - 1] = stackPtr[top - 1] > stackPtr[top] || juce::approximatelyEqual(stackPtr[top - 1], stackPtr[top]);
				--top;
				break;

			case power: stackPtr[top - 1] = std::pow(stackPtr[top - 1], stackPtr[top]);
				--top;
				break;

			case sqrt: stackPtr[top] = std::sqrt(stackPtr[top]);
				break;
			case cbrt: stackPtr[top] = std::cbrt(stackPtr[top]);
				break;

			case exp: stackPtr[top] = std::exp(stackPtr[top]);
				break;
			case exp2: stackPtr[top] = std::exp2(stackPtr[top]);
				break;
			case log: stackPtr[top] = std::log(stackPtr[top]);
				break;
			case log2: stackPtr[top] = std::log2(stackPtr[top]);
				break;
			case log10: stackPtr[top] = std::log10(stackPtr[top]);
				break;


			case sine: stackPtr[top] = std::sin(stackPtr[top]);
				break;
			case cosine: stackPtr[top] = std::cos(stackPtr[top]);
				break;
			case tangent: stackPtr[top] = std::tan(stackPtr[top]);
				break;
			case arcsine: stackPtr[top] = std::asin(stackPtr[top]);
				break;
			case arccosine: stackPtr[top] = std::acos(stackPtr[top]);
				break;
			case arctangent: stackPtr[top] = std::atan(stackPtr[top]);
				break;

			case numberConstant: stackPtr[++top] = numberConstants[nextNum++];
				break;

			case pi: stackPtr[++top] = juce::MathConstants<double>::pi;
				break;
			case twopi: stackPtr[++top] = juce::MathConstants<double>::twoPi;
				break;
			case halfpi: stackPtr[++top] = juce::MathConstants<double>::halfPi;
				break;
			case e: stackPtr[++top] = juce::MathConstants<double>::euler;
				break;

			case random: stackPtr[++top] = (double)std::rand() / RAND_MAX;
				break;

			case fs: stackPtr[++top] = globalValues.fs;
				break;
			case f: stackPtr[++top] = globalValues.f;
				break;
			case ps: stackPtr[++top] = globalValues.ps;
				break;
			case p: stackPtr[++top] = globalValues.p;
				break;
			case rs: stackPtr[++top] = globalValues.rs;
				break;
			case r: stackPtr[++top] = globalValues.r;
				break;
			case n: stackPtr[++top] = globalValues.n;
				break;
			case t: stackPtr[++top] = globalValues.t;
				break;

			case nf: stackPtr[++top] = globalValues.nf;
				break;
			case sr: stackPtr[++top] = globalValues.sr;
				break;
			case bps: stackPtr[++top] = globalValues.bps;
				break;



			case a: stackPtr[++top] = inputValues[0];
				break;
			case b: stackPtr[++top] = inputValues[1];
				break;
			case c: stackPtr[++top] = inputValues[2];
				break;
			case d: stackPtr[++top] = inputValues[3];
				break;


			case lparenthesis: break;
			case rparenthesis: break;
			case error: break;
			default: ;
			}
			jassert(top >= 0 && top < processingStack.size());
		}


		auto result = processingStack[top];

		return isinf(result) || isnan(result) ? 0.0 : result;
	}


private:
	static Op getTokenFromString(std::string const& buffer)
	{

		//if (buffer == "t") return r;

		for (const auto& p : tokens)
		{
			if (buffer == p.str)
			{
				return p.op;
			}
		}

		return error;
	}


	static bool tokenize(juce::StringRef expressionString, std::vector<Op>& tokenSequence,
	              std::vector<double>& numberConstants)
	{
		if (expressionString.isEmpty()) return true;


		auto charPtr = expressionString.text;

		Op token = error;

		State state = newToken;

		std::string buffer;

		auto currentChar = charPtr.getAndAdvance();

		bool notFinished = true;

		while (notFinished)
		{
			if (charPtr.isEmpty()) notFinished = false;

			if (state == newToken)
			{
				buffer.clear();

				if (currentChar == 0) break;

				if (!juce::CharacterFunctions::isPrintable(currentChar) ||
					juce::CharacterFunctions::isWhitespace(currentChar) ||
					currentChar > '~') //ignore character if it is not printable, whitespace or outside of ascii;
				{
					currentChar = charPtr.getAndAdvance();
					continue;
				}

				if (currentChar == '-')
					state = minusRead;

				else if (juce::CharacterFunctions::isDigit(currentChar) || currentChar == '.')
					state = readNumber;

				else if (juce::CharacterFunctions::isLetter(currentChar))
					state = readWord;

				else
					state = readSymbols;


				buffer += (char)currentChar;
			}

			switch (state)
			{
			case newToken: break;

			case minusRead:
				currentChar = charPtr.getAndAdvance();
				if (juce::CharacterFunctions::isDigit(currentChar) || currentChar == '.')
				{
					state = readNumber;
				}
				else
				{
					tokenSequence.push_back(subtract);

					state = newToken;
				}

				break;


			case readNumber:

				currentChar = charPtr.getAndAdvance();

				if (juce::CharacterFunctions::isDigit(currentChar) || currentChar == '.' || currentChar == 'x')
				{
					buffer += (char)currentChar;
				}
				else
				{
					double value = 0;
					try
					{
						value = std::stod(buffer);
					}
					catch (...)
					{
						return false;
					}


					numberConstants.push_back(value);
					tokenSequence.push_back(numberConstant);

					state = newToken;
				}

				break;


			case readWord:

				currentChar = charPtr.getAndAdvance();

				if (juce::CharacterFunctions::isLetter(currentChar) || juce::CharacterFunctions::isDigit(currentChar))
				{
					buffer += (char)currentChar;
				}
				else
				{
					token = getTokenFromString(buffer);


					if (token != error)
						tokenSequence.push_back(token);
					else
						return false;

					state = newToken;
				}

				break;


			case readSymbols:

				currentChar = charPtr.getAndAdvance();

				token = error;
				if (juce::CharacterFunctions::isPrintable(currentChar) &&
					!juce::CharacterFunctions::isWhitespace(currentChar))
				{
					buffer += (char)currentChar;
					token = getTokenFromString(buffer);
				}


				if (token == error)
					token = getTokenFromString(buffer.substr(0, 1));
				else
					currentChar = charPtr.getAndAdvance();

				if (token != error)
					tokenSequence.push_back(token);
				else
					return false;

				state = newToken;

				break;
			}
		}

		return true;
	}


	static int parsePostfix(const std::vector<Op>& tokenSequence)
	{
		int currentStackSize = 0;
		int maxStackSize = 0;


		for (auto op : tokenSequence)
		{
			//bounds checking
			if (op < invert || op >= lparenthesis) return false;

			//The net change in stack size of an op is 1 minus its arity
			currentStackSize += 1 - tokens[op].arity;

			//check for stack underflow
			if (currentStackSize < 1) return false;

			if (maxStackSize < currentStackSize) maxStackSize = currentStackSize;
		}

		//Return the maxStackSize if sequence is grammatically correct
		if (currentStackSize == 1)
		{
			return maxStackSize;
		}

		return false;
	}



	bool infixToPostfix(std::vector<Op>& tokenSequence) const
	{
		std::vector<Op> postfix;
		std::vector<Op> stack;
		stack.reserve(tokenSequence.size());
		postfix.reserve(tokenSequence.size());

		for (auto op : tokenSequence)
		{
			//bounds checking
			if (op < invert || op >= error) return false;

			auto opArity = tokens[op].arity;

			if (opArity == 0)
				postfix.push_back(op);

			else if (opArity == 1 || op == lparenthesis)
				stack.push_back(op);

			else if (opArity == 2)
			{
				while (!stack.empty())
				{
					auto op2 = stack.back();
					if (op2 == lparenthesis) break;
					if (tokens[op2].prec < tokens[op].prec || tokens[op2].prec == tokens[op].prec && tokens[op].assoc ==
						left)
					{
						postfix.push_back(op2);
						stack.pop_back();
					}
					else break;
				}

				stack.push_back(op);
			}
			else if (op == rparenthesis)
			{
				while (true)
				{
					if (stack.empty()) return false; //Mismatched parens
					auto op2 = stack.back();
					if (op2 == lparenthesis)
					{
						stack.pop_back();
						break;
					}

					postfix.push_back(op2);
					stack.pop_back();
				}
			}
		}
		while (!stack.empty())
		{
			auto op = stack.back();
			stack.pop_back();
			if (op == lparenthesis) return false; //Mismatched parens
			postfix.push_back(op);
		}

		postfix.shrink_to_fit();

		std::swap(tokenSequence, postfix);

		return true;
	}

	std::vector<Op> byteCode;
	std::vector<double> numberConstants;
	std::vector<double> processingStack;
};
