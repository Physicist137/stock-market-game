#include <map>
#include <vector>
#include <memory>
#include <random>
#include <iostream>
#include <iomanip>
#include <string>


class Stock;
class Bot;
class Market;


class Money {
	int64_t _value;

public:
	Money() : _value(0) {}
	
	inline int64_t integer() const {return _value;}
	inline double value() const {return static_cast<double>(_value) / 100.0;}
};


class Stock {
	int _id;
	int64_t _price;
	int _amount;
	double _deviation;
	double _drift;
	friend class Market;


public:
	Stock() : _id(), _price(), _amount(), _deviation(), _drift() {}

	Stock(int id, int64_t price, int amount, double deviation, double drift)
	: _id(id), _price(price), _amount(amount), _deviation(deviation), _drift(drift) {}

	inline unsigned id() const {return _id;}
	inline int64_t integer_price() const {return _price;}
	inline double price() const {return static_cast<double>(_price) / 100.0;}
	inline unsigned amount() const {return _amount;}
	
	// Should this go public or not?
	inline double deviation() const { return _deviation;}
};

struct Order {
	int id;
	int amount;
	int type;
	
	struct Type {
		constexpr static int Buy = 1;
		constexpr static int Sell = 2;
	};
	
	Order(int i, int a, int t) : id(i), amount(a), type(t) {}
};


class Bot {
	std::string _name;
	int64_t _money;
	std::vector<Stock*> _stocks;
	std::vector<int> _assets;
	std::vector<Order> _orders;
	int _day;
	friend class Market;

public:
	Bot() : _name(), _money(), _stocks(), _assets(), _orders(), _day() {}
	Bot(const std::string& str) : _name(str), _money(), _stocks(), _assets(), _orders(), _day() {}
	~Bot() = default;
	
	// Get name.
	const std::string& name() const {return _name;}
	
	// Buy and sell.
	inline void buy(int id, int amount) { _orders.push_back(Order(id, amount, Order::Type::Buy)); }
	inline void sell(int id, int amount) { _orders.push_back(Order(id, amount, Order::Type::Sell)); }
	
	// Get current day.
	inline int day() const { return _day; }
	
	// How many assets the bot has.
	inline int get_amount(int id) {return _assets[id];}
	inline const std::vector<int>& all_assets() const {return _assets;}
	
	// Stock functions.
	inline Stock* get_stock(int id) const {return _stocks[id];}
	inline const std::vector<Stock*>& all_stocks() const {return _stocks;}
	inline int amount_stocks() const {return _stocks.size();}
	
	// Amount of money available.
	inline int64_t integer_money() const {return _money;}
	inline double money() const {return static_cast<double>(_money) / 100.0;}
	
	// Bot net worth.
	int64_t integer_net() const;
	double net() const;
	
	// Trade function.
	virtual void trade() = 0;
};


int64_t Bot::integer_net() const {
	uint64_t total = 0;
	for (int i = 0; i < _assets.size(); ++i) total += _stocks[i]->integer_price() * _assets[i];
	return total + _money;
}


double Bot::net() const {
	return static_cast<double>(this->integer_net()) / 100.0;
}


class Market {
public:
	std::vector<std::unique_ptr<Stock>> _stocks;
	std::vector<Bot*> _bots;
	std::random_device _device;
    std::mt19937 _generator;
	int _day;
	int64_t _initial;

protected:
	constexpr static double market_anual_drift = 0.05;
	constexpr static double noise_factor = 0.2;
	constexpr static double drift_factor = 200.0;
	static double market_daily_drift();

public:
	Market() : _stocks(), _bots(), _device(), _generator(_device()), _day(0), _initial() {}

	void add_bot(Bot* bot);
	void create_uniform(int amount, int64_t initial_price, int initial_amount);
	void create(int amount);
	void initialize_bots(int64_t initial_money);
	void simulate();
	void bots_display();
};

double Market::market_daily_drift() {
	return std::pow(1.0 + market_anual_drift, 1.0 / 365.0) - 1.0;
}

void Market::add_bot(Bot* bot) {
	_bots.push_back(bot);
}

// (1 + b)^365 = 1+X   -->  b = (1+X)^{1/365} - 1
void Market::create_uniform(int amount, int64_t initial_price, int initial_amount) {
	std::normal_distribution<double> normal_distribution(0.0, 1.0);
	
	int latest = _stocks.size();
	for (int i = 0; i < amount; ++i) {
		int id = latest + i;
		double drift = Market::market_daily_drift() * (1.0 + drift_factor * normal_distribution(_generator));
		double deviation = noise_factor * std::abs(normal_distribution(_generator) * market_daily_drift());
		_stocks.emplace_back(new Stock(id, initial_price, initial_amount, drift, deviation));
	}
}


void Market::create(int amount) {
	std::uniform_int_distribution<std::mt19937::result_type> initial_price(100, 1000*100);
	std::uniform_int_distribution<std::mt19937::result_type> initial_amount(10, 1000);
	std::normal_distribution<double> normal_distribution(0.0, 1.0);
	
	int latest = _stocks.size();
	for (int i = 0; i < amount; ++i) {
		int id = latest + i;
		double drift = Market::market_daily_drift() * (1.0 + drift_factor * normal_distribution(_generator));
		double deviation = noise_factor * std::abs(normal_distribution(_generator) * market_daily_drift());
		_stocks.emplace_back(new Stock(id, initial_price(_generator), initial_amount(_generator), drift, deviation));
	}
}


void Market::initialize_bots(int64_t initial_money) {
	for (Bot* bot : _bots) {
		bot->_money = initial_money;
		for (auto& stock : _stocks) {
			bot->_stocks.push_back(stock.get());
			bot->_assets.push_back(0);
		}
	}
	
	_initial = initial_money;
}


void Market::simulate() {
	std::normal_distribution<double> normal_distribution(0.0, 1.0);
	
	// Simulate market.
	for (auto& stock : _stocks) {
		if (stock->_price == 0) continue;
		double factor = stock->_drift + stock->_deviation * normal_distribution(_generator);
		int64_t price_increment = static_cast<int64_t>(std::round(stock->_price * factor));
		stock->_price += price_increment;
		if (stock->_price < 0) stock->_price = 0;
	}
	
	
	// Trade.
	++_day;
	for (Bot* bot : _bots) {
		bot->trade();
		bot->_day = _day;
	}
	
	
	// Process trading orders in sequence.
	for (Bot* bot : _bots) {
		for (const Order& order : bot->_orders) {
			
			// Get order and find operation price.
			int64_t operation_price = order.amount * _stocks[order.id]->_price;
			
			if (order.type == Order::Type::Buy) {
				// Block operation and continue.
				if (_stocks[order.id]->_amount < order.amount) continue;
				if (operation_price > bot->_money) continue;
				
				// Everything went nice. Process operation.
				_stocks[order.id]->_amount -= order.amount;
				bot->_assets[order.id] += order.amount;
				bot->_money -= operation_price;
			}
			
			else if (order.type == Order::Type::Sell) {
				// Block operation.
				if (bot->_assets[order.id] < order.amount) continue;
				
				// Everything went nice. Process operation.
				_stocks[order.id]->_amount += order.amount;
				bot->_assets[order.id] -= order.amount;
				bot->_money += operation_price;
			}
			
			else continue;
		}
		
		// Clear all orders.
		bot->_orders.clear();
	}
}

// 100 * (1+X) = 101.  -->  1+X = 101/100
void Market::bots_display() {
	std::cout << std::setprecision(2) << std::fixed;
	for (Bot* bot : _bots) {
		double initial = static_cast<double>(_initial) / 100.0;
		double ratio =  bot->net() / initial - 1.0;
		std::cout << bot->name() << " assets: \tKSN " << bot->net();
		std::cout << "\tYield: ";
		if (ratio < 0) std::cout << 100.0 * ratio << "\%";
		else std::cout << "+" << 100.0 * ratio << "\%";
		std::cout << std::endl;
	}
}

// ------------------------------------------------------------------------------





class MyBot : public Bot {
public:
	MyBot() : Bot("MyBot") {}
	void trade();
};

void MyBot::trade() {
	// Do nothing all day.
}




class MyOtherBot : public Bot {
public:
	MyOtherBot() : Bot("MyOtherBot") {}
	void trade();
};

void MyOtherBot::trade() {
	// Do nothing all day.
}








// ------------------------------------------------------------------------------


int main() {
	// Create market.
	Market market;
	market.create_uniform(700, 10000, 120);
	
	// Add bots.
	market.add_bot(new MyBot);
	market.add_bot(new MyOtherBot);
	market.initialize_bots(100000);
	
	// Simulate five years of trading.
	for (int t = 0; t < 365*5; ++t) market.simulate();
	
	// Declares winner.
	market.bots_display();
}