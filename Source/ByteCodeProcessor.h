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
	float t = 0.f;
	float h = 0.f;
	float n = 0.f;
	float bpm = 0.f;
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


		//std::swap(byteCode, tokenSequence); //Make threadsafe
		//std::swap(numberConstants, nums);

		return true;
	}


	float process(float* inputValues, CounterValues& counterValues)
	{
		//DBG(inputValues[0]);
		//return (std::sin(  counterValues.n / 256 * juce::MathConstants<float>::twoPi) + 1) * inpuValues[0];
		return (static_cast<int>(counterValues.n * inputValues[0]) & 255);
		//return inputValues[0];
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
