#pragma once

#include "Behavior.h"
#include <functional>
#include <set>

class FuzzyLogic : public Composite
{
public:

	struct MembershipFunction
	{
		MembershipFunction() {}
		virtual ~MembershipFunction() {}

		// given an input value, return a truth value between 0.0 and 1.0
		virtual float operator()(float a_input) const = 0;

		// return a truth value between 0.0 and 1.0
		virtual float operator()() const = 0;

		// push any owned subfunctions into a vector before deleting
		virtual void cleanUp(std::set<MembershipFunction*>& a_toDelete) {}
	};
	struct Constant : public MembershipFunction
	{
		Constant(float a_val = 0.0f) : val(a_val) {}
		virtual ~Constant() {}

		virtual float operator()(float a_input) const { return val; }
		virtual float operator()() const { return val; }

		float val;
	};
	struct Not : public MembershipFunction
	{
		Not(MembershipFunction* a_fun = nullptr)
		: fun(a_fun) {}
		virtual ~Not() {}

		virtual float operator()(float a_input) const
		{
			return (nullptr == fun ? 1.0f - a_input : 1.0f - (*fun)(a_input));
		}
		virtual float operator()() const
		{
			return (nullptr == fun ? 1.0f : 1.0f - (*fun)());
		}
		virtual void cleanUp(std::set<MembershipFunction*>& a_toDelete)
		{
			if (nullptr != fun)
			{
				a_toDelete.insert(fun);
				fun = nullptr;
			}
		}

		MembershipFunction* fun;
	};
	struct And : public MembershipFunction
	{
		And(MembershipFunction* a_fun1 = nullptr,
			MembershipFunction* a_fun2 = nullptr)
			: fun1(a_fun1), fun2(a_fun2) {}
		virtual ~And() {}

		virtual float operator()(float a_input) const
		{
			return fmin(nullptr == fun1 ? a_input : (*fun1)(a_input),
						nullptr == fun2 ? a_input : (*fun2)(a_input));
		}
		virtual float operator()() const
		{
			return fmin(nullptr == fun1 ? 0.0f : (*fun1)(),
						nullptr == fun2 ? 0.0f : (*fun2)());
		}
		virtual void cleanUp(std::set<MembershipFunction*>& a_toDelete)
		{
			if (nullptr != fun1)
			{
				a_toDelete.insert(fun1);
				fun1 = nullptr;
			}
			if (nullptr != fun2)
			{
				a_toDelete.insert(fun2);
				fun2 = nullptr;
			}
		}

		MembershipFunction* fun1;
		MembershipFunction* fun2;
	};
	struct Or : public MembershipFunction
	{
		Or(MembershipFunction* a_fun1 = nullptr,
		MembershipFunction* a_fun2 = nullptr)
		: fun1(a_fun1), fun2(a_fun2) {}
		virtual ~Or() {}

		virtual float operator()(float a_input) const
		{
			return fmax(nullptr == fun1 ? a_input : (*fun1)(a_input),
						nullptr == fun2 ? a_input : (*fun2)(a_input));
		}
		virtual float operator()() const
		{
			return fmax(nullptr == fun1 ? 0.0f : (*fun1)(),
						nullptr == fun2 ? 0.0f : (*fun2)());
		}
		virtual void cleanUp(std::set<MembershipFunction*>& a_toDelete)
		{
			if (nullptr != fun1)
			{
				a_toDelete.insert(fun1);
				fun1 = nullptr;
			}
			if (nullptr != fun2)
			{
				a_toDelete.insert(fun2);
				fun2 = nullptr;
			}
		}

		MembershipFunction* fun1;
		MembershipFunction* fun2;
	};
	struct Very : public MembershipFunction
	{
		Very(MembershipFunction* a_fun = nullptr)
			: fun(a_fun) {}
		virtual ~Very() {}

		virtual float operator()(float a_input) const
		{
			float value = (nullptr == fun ? a_input : (*fun)(a_input));
			return value * value;
		}
		virtual float operator()() const
		{
			float value = (nullptr == fun ? 0.0f : (*fun)());
			return value * value;
		}
		virtual void cleanUp(std::set<MembershipFunction*>& a_toDelete)
		{
			if (nullptr != fun)
			{
				a_toDelete.insert(fun);
				fun = nullptr;
			}
		}

		MembershipFunction* fun;
	};
	struct Somewhat : public MembershipFunction
	{
		Somewhat(MembershipFunction* a_fun = nullptr)
			: fun(a_fun) {}
		virtual ~Somewhat() {}

		virtual float operator()(float a_input) const
		{
			float not = 1.0f - (nullptr == fun ? a_input : (*fun)(a_input));
			return 1.0f - (not * not);
		}
		virtual float operator()() const
		{
			float not = (nullptr == fun ? 1.0f : 1.0f - (*fun)());
			return 1.0f - (not * not);
		}
		virtual void cleanUp(std::set<MembershipFunction*>& a_toDelete)
		{
			if (nullptr != fun)
			{
				a_toDelete.insert(fun);
				fun = nullptr;
			}
		}

		MembershipFunction* fun;
	};
	struct Indeed : public MembershipFunction
	{
		Indeed(MembershipFunction* a_fun = nullptr)
			: fun(a_fun) {}
		virtual ~Indeed() {}

		virtual float operator()(float a_input) const
		{
			return evaluate(nullptr == fun ? 0.0f : (*fun)(a_input));
		}
		virtual float operator()() const
		{
			return evaluate(nullptr == fun ? 0.0f : (*fun)());
		}
		virtual void cleanUp(std::set<MembershipFunction*>& a_toDelete)
		{
			if (nullptr != fun)
			{
				a_toDelete.insert(fun);
				fun = nullptr;
			}
		}

		MembershipFunction* fun;

	private:
		float evaluate(float a_value) const
		{
			if (0.5f <= a_value)
				return 2 * a_value * a_value;
			float not = 1.0f - a_value;
			return 1.0f - (2 * not * not);
		}
	};
	struct Exactly : public MembershipFunction
	{
		Exactly(float a_peak)
			: val([] { return 0.0f; }), peak(a_peak) {}
		Exactly(std::function<float(void)> a_val, float a_peak)
			: val(a_val), peak(a_peak) {}
		virtual ~Exactly() {}

		virtual float operator()(float a_input) const
		{
			return (peak == a_input ? 1.0f : 0.0f);
		}
		virtual float operator()() const
		{
			float value = val();
			return (value == peak ? 1.0f : 0.0f);
		}

		std::function<float(void)> val;
		float peak;
	};
	struct Triangle : public MembershipFunction
	{
		Triangle(float a_upperBound, float a_lowerBound, float a_peak)
			: val([] { return 0.0f; }), upperBound(a_upperBound),
			  lowerBound(a_lowerBound), peak(a_peak) {}
		Triangle(float a_upperBound = 1.0f, float a_lowerBound = 0.0f)
			: val([] { return 0.0f; }), upperBound(a_upperBound), lowerBound(a_lowerBound),
			  peak((a_upperBound + a_lowerBound) / 2) {}
		Triangle(std::function<float(void)> a_val,
				 float a_upperBound, float a_lowerBound, float a_peak)
			: val(a_val), upperBound(a_upperBound),
			  lowerBound(a_lowerBound), peak(a_peak) {}
		Triangle(std::function<float(void)> a_val,
				 float a_upperBound = 1.0f, float a_lowerBound = 0.0f)
			: val(a_val), upperBound(a_upperBound), lowerBound(a_lowerBound),
			  peak((a_upperBound + a_lowerBound) / 2) {}
		virtual ~Triangle() {}

		virtual float operator()(float a_input) const
		{
			return evaluate(a_input);
		}
		virtual float operator()() const
		{
			return evaluate(val());
		}

		std::function<float(void)> val;
		float upperBound;
		float lowerBound;
		float peak;

	private:
		float evaluate(float a_value) const
		{
			return (peak == a_value ? 1.0f
				: a_value <= lowerBound || upperBound <= a_value ? 0.0f
				: a_value < peak ? (a_value - lowerBound) / (peak - lowerBound)
				: (upperBound - a_value) / (upperBound - peak));
		}
	};
	struct Trapezoid : public MembershipFunction
	{
		Trapezoid(float a_peakUpperBound, float a_peakLowerBound,
				  float a_upperBound = 1.0f, float a_lowerBound = 0.0f)
			: val([] { return 0.0f; }), upperBound(a_upperBound), lowerBound(a_lowerBound),
			  peakUpperBound(a_peakUpperBound), peakLowerBound(a_peakLowerBound) {}
		Trapezoid(std::function<float(void)> a_val,
				  float a_peakUpperBound, float a_peakLowerBound,
				  float a_upperBound = 1.0f, float a_lowerBound = 0.0f)
			: val(a_val), upperBound(a_upperBound), lowerBound(a_lowerBound),
			  peakUpperBound(a_peakUpperBound), peakLowerBound(a_peakLowerBound) {}
		virtual ~Trapezoid() {}

		virtual float operator()(float a_input) const
		{
			return evaluate(a_input);
		}
		virtual float operator()() const
		{
			return evaluate(val());
		}

		std::function<float(void)> val;
		float upperBound;
		float lowerBound;
		float peakUpperBound;
		float peakLowerBound;

	private:
		float evaluate(float a_value) const
		{
			return (peakLowerBound <= a_value && a_value <= peakUpperBound ? 1.0f
				: a_value <= lowerBound || upperBound <= a_value ? 0.0f
				: a_value < peakLowerBound ? (a_value - lowerBound) / (peakLowerBound - lowerBound)
				: (upperBound - a_value) / (upperBound - peakUpperBound));
		}
	};
	struct LeftShoulder : public MembershipFunction
	{
		LeftShoulder(float a_peak = 0.0f, float a_trough = 1.0f)
			: val([] { return 0.0f; }), trough(a_trough), peak(a_peak) {}
		LeftShoulder(std::function<float(void)> a_val,
					 float a_peak = 0.0f, float a_trough = 1.0f)
			: val(a_val), trough(a_trough), peak(a_peak) {}
		virtual ~LeftShoulder() {}

		virtual float operator()(float a_input) const
		{
			return evaluate(a_input);
		}
		virtual float operator()() const
		{
			return evaluate(val());
		}

		std::function<float(void)> val;
		float trough;
		float peak;

	private:
		float evaluate(float a_value) const
		{
			return (a_value <= peak ? 1.0f : trough <= a_value ? 0.0f
					: (trough - a_value) / (trough - peak));
		}
	};
	struct RightShoulder : public MembershipFunction
	{
		RightShoulder(float a_peak = 1.0f, float a_trough = 0.0f)
			: val([] { return 0.0f; }), trough(a_trough), peak(a_peak) {}
		RightShoulder(std::function<float(void)> a_val,
					  float a_peak = 1.0f, float a_trough = 0.0f)
			: val(a_val), trough(a_trough), peak(a_peak) {}
		virtual ~RightShoulder() {}

		virtual float operator()(float a_input) const
		{
			return evaluate(a_input);
		}
		virtual float operator()() const
		{
			return evaluate(val());
		}

		std::function<float(void)> val;
		float trough;
		float peak;

	private:
		float evaluate(float a_value) const
		{
			return (a_value >= peak ? 1.0f : trough >= a_value ? 0.0f
					: (a_value - trough) / (peak - trough));
		}
	};
	struct Distribution : public MembershipFunction
	{
		Distribution(std::function<float(float)> a_dist, float a_scale = 1.0f)
			: val([] { return 0.0f; }), dist(a_dist), scale(a_scale) {}
		Distribution(std::function<float(void)> a_val,
					 std::function<float(float)> a_dist, float a_scale = 1.0f)
			: val(a_val), dist(a_dist), scale(a_scale) {}
		virtual ~Distribution() {}

		virtual float operator()(float a_input) const
		{
			return scale * dist(a_input);
		}
		virtual float operator()() const
		{
			return scale * dist(val());
		}

		std::function<float(void)> val;
		std::function<float(float)> dist;
		float scale;
	};

	FuzzyLogic() {}
	virtual ~FuzzyLogic() {}

	virtual bool execute(Agent* a_agent)
	{
		// if there are no rules, return true (since nothing fails)
		if (0 == m_rules.size())
			return true;

		double numerator = 0.0;
		double denominator = 0.0;
		for (auto rule : m_rules)
		{
			// fuzzify
			float dom = (nullptr == rule.fun ? 0.0f : (*rule.fun)());

			// prepare for average of maxima method
			numerator += (double)dom * (double)rule.maximum;
			denominator += dom;
		}

		// calculated preference
		float preference = (0.0 == denominator ? 0.0f : (float)(numerator / denominator));

		// calculate random numbers required for choosing each rule
		std::vector<float> odds;
		for (auto rule : m_rules)
		{
			odds.push_back((odds.empty() ? 0.0f : odds.back()) + rule.dist(preference));
		}

		// choose a rule to follow
		float choice = odds.back() * (float)rand() / RAND_MAX;
		for (unsigned int i = 0; i < m_rules.size(); ++i)
		{
			// if the rule exists, return its result (or true if the choice is doing nothing)
			if (choice <= odds[i])
				return (nullptr == m_children[i] ? true
						: m_children[i]->execute(a_agent));
		}

		// if none of the rules were chosen, return true
		return true;
	}

	void addRule(Behavior* a_behavior, MembershipFunction* a_fun,
				 std::function<float(float)> a_dist, float a_maximum)
	{
		m_rules.push_back(Rule(a_fun, a_dist, a_maximum));
		m_children.push_back(a_behavior);
	}

	void destroyRules()
	{
		// get ready to find all membership functions in the heirarchy
		std::set<MembershipFunction*> toCheck;
		std::set<MembershipFunction*> toDelete;
		for (auto rule : m_rules)
			toCheck.insert(rule.fun);

		// iterate through functions that haven't been checked for subfunctions
		while (!toCheck.empty())
		{
			MembershipFunction* current = *toCheck.begin();
			toCheck.erase(current);
			if (0 == toDelete.count(current))
			{
				current->cleanUp(toCheck);
				toDelete.insert(current);
			}
		}

		// delete all membership functions
		while (!toDelete.empty())
		{
			MembershipFunction* current = *toDelete.begin();
			toDelete.erase(current);
			delete current;
		}
	}

protected:

	struct Rule
	{
		MembershipFunction* fun;
		std::function<float(float)> dist;
		float maximum;

		Rule(MembershipFunction* a_fun = nullptr,
			 std::function<float(float)> a_dist = [](float x) { return 0.0f; },
			 float a_maximum = 0.0)
			: fun(a_fun), dist(a_dist), maximum(a_maximum) {}
	};

	std::vector<Rule> m_rules;
};
