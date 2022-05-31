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


	std::vector<Op> byteCode;
	std::vector<float> numberConstants;
};
