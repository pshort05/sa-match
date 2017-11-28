/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.cc
 * Copyright (C) 2017 Paul Short <paul@jpkweb.com>
 *
 * sa-match is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Built with C++14
 */

#include <iostream>
#include <map>
#include <algorithm>
#include <vector>
#include <string>
#include <list>

using namespace std;


// for c standard library calls
#include <cstring>


// Some standard defintions for the program
#define MAX_SYMBOL_SIZE		36

// -----------------------------------------------------------------------------
// Function to get the current time and covert to a double for keeping the book
// order by time - this needs to be updated based on the type of hardware in use
// -----------------------------------------------------------------------------
#include <ctime> // needed for clock_gettime()
void makeTime( double *currentTime ) {
	struct timespec now;

	clock_gettime( CLOCK_REALTIME, &now );
	*currentTime = now.tv_sec + (now.tv_nsec*(0.000000001));
};

// -----------------------------------------------------------------------------
// This is all the data we track for an order: keep pointers to this in the code
// -----------------------------------------------------------------------------
struct orderData {
	int		order_id;
	double  price;
	double  time;
	char	side;
	int		size;
	char	symbol[MAX_SYMBOL_SIZE];
	short   exchange;
	orderData( int n) : order_id(n) {}
 };

// Sort functions for the orderData vector
bool sortSellSideData( orderData &a, orderData &b) {
	return a.price>b.price;
}

bool sortBuySideData( orderData &a, orderData &b ) {
	return a.price<b.price;
}


// -----------------------------------------------------------------------------
// This will keep the data for each side of the book
// -----------------------------------------------------------------------------
struct priceData {
	double price;
	int size;
	int orderNum;
	std::list<struct orderData*> orders;
	priceData( double n ) : price(n) {}
};

struct priceDataFinder {
	priceDataFinder( double key ) : key_(key) {}
	bool operator()(const priceData & obj) const {
		return key_==obj.price;
	}
	const double key_;
};

bool sortSellSidePrices( priceData &a, priceData &b ) {
	return a.price>b.price;
}

bool sortBuySidePrices( priceData &a, priceData &b ) {
	return a.price<b.price;
}


// -----------------------------------------------------------------------------
// Order book class
// -----------------------------------------------------------------------------
class OrderBook {
	 	public :
		OrderBook( char* symbol );
		~OrderBook();		void add(int order_id, char side, double price, int size);
		void modify(int order_id, int new_size);
		void remove(int order_id);
		double get_price(char side, int level);
		int get_size(char side, int level);
	private :
		std::map<int, struct orderData *> bookData; // Full order book data
		std::vector<struct priceData> buySide; // Sorted book data
		std::vector<struct priceData> sellSide; // Sorted book data
		char bookSymbol[MAX_SYMBOL_SIZE];
		 };

OrderBook::OrderBook( char* symbol ) {

	// for the test we will simply assign a test symbol...
	strncpy( bookSymbol, symbol, MAX_SYMBOL_SIZE );

}

OrderBook::~OrderBook( void ) {

	// destroy the order book...
}

// Add a new order into the book - keep buy and sell side orders sorted
// Keep the full order data in an unorderd map
void OrderBook::add(int order_id, char side, double price, int size) {

	struct priceData *ptrPrice;
	std::vector<struct priceData>::iterator iter;

	// Create a new order
	orderData order(order_id);
	order.side = side;
	order.price = price;
	order.size = size;
	makeTime(&order.time);
	bookData.emplace( order_id, &order );


	// Add order to the correct side of the book
	switch( side ) {
		case 'B':
		{
			// See if a price exists at this level, if so add it into the vector
			iter = std::find_if( buySide.begin(), buySide.end(), priceDataFinder(price) );
			if( iter != buySide.end() ) {
				// Add the price into an existing price level:
				iter->size += size;
				iter->orders.push_back(&order);
				iter->orderNum++;
			}
			else {
				// First order at this price level
				priceData priceLevel( price );
				priceLevel.size = size;
				priceLevel.orders.push_front(&order);
				priceLevel.orderNum=1;
				buySide.push_back( priceLevel );
				std::sort( buySide.begin(), buySide.end(), sortBuySidePrices );
			}
			break;
		}
		case 'S':
		{
			// See if a price exists at this level, if so add it into the vector
			iter = std::find_if( sellSide.begin(), sellSide.end(), priceDataFinder(price) );
			if( iter != sellSide.end() ) {
				// Add the price into an existing price level:
				iter->size += size;
			}
			else {
				priceData priceLevel( price );
				priceLevel.size = size;
				sellSide.push_back( priceLevel );
				std::sort( sellSide.begin(), sellSide.end(), sortSellSidePrices );
			}
			break;
		}
		default:
		{
			cout<< "Unknown order side - order not added" << std::endl;
			break;
		}
	}

}

int OrderBook::get_size(char side, int level) {

	switch( side ) {
		case 'B':
			return buySide[level-1].size;
		case 'S':
			return sellSide[level-1].size;
		default:
			cout<< "Unknown order side - order not found" << std::endl;
			break;
	}
}

double OrderBook::get_price( char side, int level) {

	switch( side ) {
		case 'B':
			return buySide[level-1].price;
		case 'S':
			return sellSide[level-1].price;
		default:
			cout<< "Unknown order side - order not found" << std::endl;
			break;
	}
}

void OrderBook::modify(int order_id, int new_size) {

}

void OrderBook::remove(int order_id) {

	// Look up the existing order then remove it after removing the shares from the book
	std::map<int, struct orderData *>::iterator iterBook;
	std::vector<struct priceData>::iterator iterPrice;


	iterBook = bookData.find( order_id);
	if( iterBook != bookData.end() ) {

		switch( iterBook->side ) {
			case 'B':
			{
				iter = std::find_if( buySide.begin(), buySide.end(), priceDataFinder(iterBook->price) );
				if( iter != buySide.end() ) {
					// Found the price - remove the order from the book
					// If it's the only order at this price, remove the entire price from the book
					if( iterPrice->orderNum ==1 ) {
						delete interPrice;
						std::sort( buySide.begin(), buySide.end(), sortBuySidePrices ); // Resort the price list
					}
					else {
						iterPrice->orderNum--;
						iterPrice->size -= iterBook.size;
					}
					delete interBook; // Now this will keep this in the order list but it will be NULL so we have to take that into account when matching
				}
				else {

				}
			}
			break;
			}
			case 'S':
			{
				return sellSide[level-1].price;
			}
			default:
				cout<< "Unknown order side - order not found" << std::endl;
				break;
		}
	}
}



// --------------- BEGIN MAIN ---------------
// Includes for some standard C functions
#include <stdio.h>
int main() {

	char symbolName[MAX_SYMBOL_SIZE] = "GOOG";
	double time;
	double price;
	OrderBook myOrder(symbolName);


	// Some test data during dev
	myOrder.add( 1, 'B', 45.4, 100 );
	myOrder.add( 2, 'B', 45.3, 200 );
	myOrder.add( 3, 'B', 45.4, 300 );
	myOrder.add( 4, 'B', 45.9, 100 );
	myOrder.add( 5, 'B', 45.9, 10 );
	myOrder.remove( 2 );

	printf( "1: %f size: %d\n", myOrder.get_price('B', 1), myOrder.get_size('B',1));
	printf( "2: %f size: %d\n", myOrder.get_price('B', 2), myOrder.get_size('B',2));
	printf( "3: %f size: %d\n", myOrder.get_price('B', 3), myOrder.get_size('B',3));


	std::cout << "Test" << std::endl;
	return 0;
}
