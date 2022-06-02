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

		/*  absolute,
		  min,
		  max,

		  power,*/


		sine,
		cosine,
		tangent,

		numberConstant,

		t,
		h,
		n,
		bpm,

		a,
		b,
		c,
		d,

		lparenthesis,
		rparenthesis,

		error,
	};

	enum Assoc {none=0, left, right};

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
	{"-", subtract,6, left, 2 },
	{"*", multiply, 5, left, 2},
	{"/", divide, 5, left, 2},
	{"%", modulo, 5, left, 2},

	{"~", bitnot, 2, right, 1},
	{"&", bitand, 11, left, 2},
	{"|", bitor, 13, left, 2},
	{"^", bitxor,12, left, 2},
	{"<<", lshift, 7, left, 2},
	{">>", rshift,  7, left, 2},

	{"!", not, 2, right, 1},
	{"&&", and, 14, left, 2},
	{"||", or, 15, left, 2},

	{"==", equal, 10, left, 2},
	{"!=", notequal, 10, left, 2},
	{"<", less, 9, left, 2},
	{"<=", lessorequal, 9, left, 2},
	{">", greater, 9, left, 2},
	{">=", greaterorequal, 9, left, 2},




    {"sin", sine, 2, right, 1},
    {"cos", cosine, 2, right, 1},
    {"tan", tangent, 2, right, 1},

    {"", numberConstant, 0, none, 0},

	{"t", t, 0, none, 0},
	{"h", h, 0, none, 0},
	{"n", n, 0, none, 0},
	{"bpm", bpm, 0, none, 0},

	{"a", a, 0, none, 0},
	{"b", b, 0, none, 0},
	{"c", c, 0, none, 0},
	{"d", d, 0, none, 0},



    {"(", lparenthesis, 0, none, -1},
    {")", rparenthesis, 0, none, -1}


};

	enum State { newToken, minusRead, readNumber, readWord, readSymbols };




public:
	bool update(juce::StringRef exprStr)//, juce::String& errorStr)
	{
		std::vector<Op> tokenSequence;
		std::vector<double> nums;

		if (!tokenize(exprStr, tokenSequence, nums)) return false;

		//Try to parse as postfix. If it fails, try to convert from infix to postfix, then try to parse as postfix again
		if(!parsePostfix(tokenSequence))
		{
			if (!infixToPostfix(tokenSequence)) return false;
			if (!parsePostfix(tokenSequence))  return false;
		}


		std::swap(byteCode, tokenSequence); //Make threadsafe
		std::swap(numberConstants, nums);

		return true;
	}


	float process(float* inputValues, CounterValues& counterValues)
	{
		//DBG(inputValues[0]);
		//return (std::sin(  counterValues.n / 256 * juce::MathConstants<float>::twoPi) + 1) * inpuValues[0];
		
		if (byteCode.empty()) return 0;

		std::vector<double> stack;
		stack.resize(100);
		int top = -1;

		auto* codePtr = byteCode.data();
		auto* numPtr = numberConstants.data();
		auto* stackPtr = stack.data();

		int nextNum = 0;



		for (auto op : byteCode)
		{
			switch (op)
			{
			case invert:        stackPtr[top] = -stackPtr[top];  break;
			case add:           stackPtr[top - 1] = stackPtr[top - 1] + stackPtr[top]; --top; break;
			case subtract:       stackPtr[top - 1] = stackPtr[top - 1] - stackPtr[top]; --top; break;
			case multiply:  stackPtr[top - 1] = stackPtr[top - 1] * stackPtr[top]; --top; break;
			case divide:  stackPtr[top - 1] = stackPtr[top - 1] / stackPtr[top]; --top; break;

			case modulo:  if ((int)stackPtr[top]) stackPtr[top - 1] = (int)stackPtr[top - 1] % (int)stackPtr[top]; else return 0; --top; break;

			case bitnot: stackPtr[top] = ~(int)stackPtr[top];  break;
			case bitand :  stackPtr[top - 1] = (int)stackPtr[top - 1] & (int)stackPtr[top]; --top; break;
			case bitor :  stackPtr[top - 1] = (int)stackPtr[top - 1] | (int)stackPtr[top]; --top; break;
			case bitxor:  stackPtr[top - 1] = (int)stackPtr[top - 1] ^ (int)stackPtr[top]; --top; break;
			case lshift: stackPtr[top - 1] = (int)stackPtr[top - 1] << (int)stackPtr[top]; --top; break;
			case rshift: stackPtr[top - 1] = (int)stackPtr[top - 1] >> (int)stackPtr[top]; --top; break;

			case not:  stackPtr[top] = !(int)stackPtr[top]; break;
			case and : stackPtr[top - 1] = (int)stackPtr[top - 1] && (int)stackPtr[top]; --top; break;
			case or : stackPtr[top - 1] = (int)stackPtr[top - 1] || (int)stackPtr[top]; --top; break;

			case equal:         stackPtr[top - 1] = juce::approximatelyEqual(stackPtr[top - 1], stackPtr[top]); --top; break;
			case notequal:      stackPtr[top - 1] = !juce::approximatelyEqual(stackPtr[top - 1], stackPtr[top]); --top; break;
			case less:          stackPtr[top - 1] = stackPtr[top - 1] < stackPtr[top]; --top; break;
			case lessorequal:   stackPtr[top - 1] = stackPtr[top - 1] <= stackPtr[top]; --top; break;
			case greater:       stackPtr[top - 1] = stackPtr[top - 1] > stackPtr[top]; --top; break;
			case greaterorequal:stackPtr[top - 1] = stackPtr[top - 1] >= stackPtr[top]; --top; break;

			case sine:  stackPtr[top] = std::sin(stackPtr[top]); break;
			case cosine: stackPtr[top] = std::cos(stackPtr[top]); break;
			case tangent: stackPtr[top] = std::tan(stackPtr[top]); break;

			case numberConstant: stackPtr[++top] = numberConstants[nextNum++]; break;

			case t: stackPtr[++top] = counterValues.t; break;
			case h: stackPtr[++top] = counterValues.h; break;
			case n: stackPtr[++top] = counterValues.n; break;
			case bpm: stackPtr[++top] = counterValues.bpm; break;


			case a: stackPtr[++top] = inputValues[0]; break;
			case b: stackPtr[++top] = inputValues[1]; break;
			case c: stackPtr[++top] = inputValues[2]; break;
			case d: stackPtr[++top] = inputValues[3]; break;
				

			case lparenthesis: break;
			case rparenthesis: break;
			case error: break;
			default:;
			}
		}

		/*  while (true)
		  {


			  switch (*codePtr++)
			  {
			  case invert: *stackPtr = -*stackPtr; break;
			  case add: *(stackPtr-1) = *(stackPtr-1) + *stackPtr; --stackPtr; break;
			  case subtract: break;
			  case multiply: break;
			  case divide: break;
			  case modulo: break;
			  case bitnot: break;
			  case bitand: break;
			  case bitor: break;
			  case bitxor: break;
			  case lshift: break;
			  case rshift: break;
			  case not: break;
			  case and: break;
			  case or: break;
			  case equal: break;
			  case notequal: break;
			  case less: break;
			  case lessorequal: break;
			  case greater: break;
			  case greaterorequal: break;
			  case sine: break;
			  case cosine: break;
			  case tangent: break;
			  case numberConstant: break;
			  case lparenthesis: break;
			  case rparenthesis: break;
			  case error: break;
			  default: ;
			  }
		  }*/

		fpclassify(0.0);


		auto result = stack[top];

		return isinf(result) || isnan(result) ? 0.0 : result;
	}


private:



	Op getTokenFromString(std::string const& buffer)
	{
		for (const auto& p : tokens)
		{
			if (buffer == p.str)
			{
				return p.op;
			}
		}

		return error;
	}


	bool tokenize(juce::StringRef expressionString, std::vector<Op>& tokenSequence,
	              std::vector<double>& numberConstants)
	{
		if (expressionString.isEmpty()) return true;


		auto charPtr = expressionString.text;
		//Token token = NUM;

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

				if (juce::CharacterFunctions::isLetter(currentChar))
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


	bool parsePostfix(std::vector<Op>& tokenSequence)
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

		return currentStackSize == 1;
	}


	bool infixToPostfix(std::vector<Op>& tokenSequence)
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
};
