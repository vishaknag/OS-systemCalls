#include "pro1.h"

int *orderTakerStatus = NULL;

int WAITER_COUNT = 0, COOK_COUNT = 0, CUST_COUNT = 0, OT_COUNT = 0;
int TABLE_COUNT = 20;
int tables[20];
int FILLED = 1000, MININVENTORY = 50;
int SIX$BURGER = 1, THREE$BURGER = 2, VEGBURGER = 3, FRIES = 4;
int MINCOOKEDFOOD = 2 ;
int MAXDIFF = 4;		// difference between the cooked food and the food to be cooked
List *cooksOnBreak;
int flag = 0;

// --------------------------------------------------
// Test 1 - see TestSuite() for details
// --------------------------------------------------
Semaphore t1_s1("t1_s1",0);       // To make sure t1_t1 acquires the
                                  // lock before t1_t2
Semaphore t1_s2("t1_s2",0);       // To make sure t1_t2 Is waiting on the 
                                  // lock before t1_t3 releases it
Semaphore t1_s3("t1_s3",0);       // To make sure t1_t1 does not release the
                                  // lock before t1_t3 tries to acquire it
Semaphore t1_done("t1_done",0);   // So that TestSuite knows when Test 1 is
                                  // done
Lock t1_l1("t1_l1");		  // the lock tested in Test 1


//------------------------------------------------------------------------
// DATABASES
//------------------------------------------------------------------------
// Customer database to store all the information related to a 
// particular customer
custDB custData[100];

// database to store the food to be cooked by the cooks
foodToBeCookedDB foodToBeCookedData;

// database to store the ready food cooked by the cooks
foodReadyDB foodReadyData;	

	
//------------------------------------------------------------------------
// LOCKS
//------------------------------------------------------------------------

//Lock broadcastTokenLock("broadcastTokenLock");

/* Locks - for customer-ordertaker interaction */
// Lock to establish mutual exclusion on customer line
Lock custLineLock("custLineLock");           

// Lock to establish mutual exclusion on the OT status 
// and for actions between customer and OT
Lock *orderTakerLock[100];	

// Lock to establish mutual exclusion on the foodReadyDataBase
Lock foodReadyDBLock("foodReadyDBLock");	

// Lock to establish mutual exclusion on the foodToBeCookedDatabase
Lock foodToBeCookedDBLock("foodToBeCookedDBLock");

// Lock to get the next token number to be given to the customer
Lock nextTokenNumberLock("nextTokenNumberLock");

// Lock to access the location where all money collected at the restaurant
// is stored by the Order Takers
Lock moneyAtRestaurantLock("moneyAtRestaurantLock");

// Lock to wait in the to-go waiting line
Lock waitToGoLock("waitToGoLock");

// Lock to establish mutual exclusion on the tables data
Lock tablesDataLock("tablesDataLock");

// Lock to wait in the eat-in table waiting line
Lock eatInWaitingLock("eatInWaitingLock");

// Lock for the eat-in seated customers waiting for food
Lock eatInFoodWaitingLock("eatInFoodWaitingLock");

// Lock to update the foodToBag count
Lock foodToBagLock("foodToBagLock");

// Lock to add/delete tokenNo into the List of bagged orders
Lock foodBaggedListLock("foodBaggedListLock");

// Lock to access the raw materials inventory
Lock inventoryLock("inventoryLock");

// Lock to update the manager line length monitor
Lock managerLock("managerLock");

// Lock to update the what to cook value
Lock whatToCookNextLock("whatToCookNextLock");

// Locks to set the stop cooking flags for different food items
Lock stopSix$BurgerLock("stopSix$BurgerLock");
Lock stopThree$BurgerLock("stopThree$BurgerLock");
Lock stopVegBurgerLock("stopVegBurgerLock");
Lock stopFriesLock("stopFriesLock");

// Lock for the communication between Manager and Waiter
Lock waiterSleepLock("waiterSleepLock");

// Customer data Lock
Lock customerDataLock("customerDataLock");

// Lock to enter a cook waiting queue / removing a cook from waiting queue
Lock wakeUpCookLock("wakeUpCookLock");	

// Lock to update the number of customers serviced
Lock custServedLock("custServedLock");

//------------------------------------------------------------------------
// CONDITION VARIABLES
//------------------------------------------------------------------------							
/* CV - for customer-ordertaker interaction */
// CV to sequence the customers in the customer line
// - customers to join the customer line
// - OT to remove a waiting customer from customer line
Condition custLineCV("custlineCV");    

// array of CV to sequence the actions between customer & OT
Condition *orderTakerCV[100];

// CV for the to-go customers to join the to-go waiting line
//Condition waitToGoCV("waitToGoCV");

// CV for waiting in line to find a free table
Condition tablesDataCV("tablesDataCV");

// CV for sequencing the customers waiting for the free table
Condition eatInWaitingCV("eatInWaitingCV");

// CV for sequencing the eat-in seated customers waiting for the order
Condition eatInFoodWaitingCV("eatInFoodWaitingCV");

// CV for sequencing the eat-in customers in the manager reply waiting queue
Condition managerCV("managerLock");

// CV for waiters to wait for the signal from the manager
Condition waiterSleepCV("waiterSleepCV");

// Common cook waiting queue 
Condition wakeUpCookCV("wakeUpCookCV");


// CV for the to-go customers to join the to-go waiting line
Condition toGoCV("toGoCV");
//------------------------------------------------------------------------
// GLOBAL MONITOR VARIABLES
//------------------------------------------------------------------------
int custLineLength = 0;		// to access the line status one at a time
int nextTokenNumber = 0;		// to ensure that token number is unique
int moneyAtRestaurant;		// to store the money collected 
int Refillcash;
int broadcastedTokenNo;		// to store the current broadcasted token
int managerLineLength;		// to store information on the number of eat-in
							// customers waiting for manager reply 
int tableAvailable;			// manager sets this if table is available 
int foodToBag;				// number of orders still to be bagged	
int inventory = 100;				// stores the current raw materials available
							// in the restaurant inventory
int whatToCookNext;			// tells the cook what to cook next							
int stopSix$Burger = 0;		// Flags for the cooks to stop cooking
int stopThree$Burger = 0;
int stopVegBurger = 0;
int stopFries = 0;		
int baggedList[100];		// list to store the token numbers of bagged orders
int custServed = 0;			// Counter to keep a count of customers served
int workingCooks[5];		//indicate the number of working cooks	
//------------------------------------------------------------------------
// Randomly generates the customers order and fills the customer database
//------------------------------------------------------------------------

void generateCustomerOrder() {
    int remainder = 0, index = 0;
	long int randInt = 0; 
	int minVal = 1, maxVal = 63, rand_i = 0;
	int range = (maxVal - minVal) + 1;
	int bit[7];
	   
	for(rand_i = 1; rand_i <= CUST_COUNT; rand_i++){ 
		// generate random number
		
		srand(rand_i);
		randInt = (rand() % range);
	    for(index = 1; index < 7; index ++){
		    remainder = randInt % 2;
		    randInt = randInt >> 1;
		    bit[index] = remainder;
	    }
	// set the generated random order into customer database
	
	custData[rand_i].six$Burger = bit[1];
	custData[rand_i].three$Burger = bit[2];
	custData[rand_i].vegBurger = bit[3];
	custData[rand_i].fries = bit[4];
	custData[rand_i].soda = bit[5];
		if(rand_i%2 == 0){
			custData[rand_i].dineType = 1; 
		}else{
			custData[rand_i].dineType = 0;
		}	
	}
}


/* Customer thread body */  

void Customer(int myIndex)
{
    int index = 0;
	int internalOT = 0;
	int managerOT = 0;
	
    // Acquire this LOCK to become the NEXT eligible customer
	custLineLock.Acquire();
	// Check if any Order Taker is FREE
	// - if any then make him as the Order Taker for the current customer
	// - Make that Order Taker status as BUSY
	for(index = 1; index <= OT_COUNT; index++){ 
		if(orderTakerStatus[index] == OT_FREE){	
			orderTakerStatus[index] = OT_BUSY;
			custData[myIndex].myOT = index;
			break;
		}
	}
	
	//If no Order Takers are free
	// - increment the linelength monitor variable
	// - then wait in the customer waiting line
	if(custData[myIndex].myOT == 0){
		custLineLength++;
		custLineCV.Wait(&custLineLock);
	}
	// Customer is here because he received a signal from the Order Taker
	// - customer has also acquired the custLineLock

	// Check for the Order Taker who is WAITING 
	// - make him as the Order Taker for the current customer
	// - Make that Order Taker status as BUSY
	if(custData[myIndex].myOT == 0){
		for(index = 1; index <= OT_COUNT; index++){ 
			if(orderTakerStatus[index] == OT_WAIT){	
				orderTakerStatus[index] = OT_BUSY;
				custData[myIndex].myOT = index;
				break;
			}
		}
	}	
	// If No Order Taker with waiting status(2) is found then its the 
	// Manager who is doing the job of the Order Taker
	//	if(custData[myIndex].myOT == 0){
		// Manager is my Order Taker
	//		internalOT = OT_COUNT + 1;
		// Flag to indicate that manager is my OT
	//		managerOT = 1;
	//	}	

	// Releasing the custLineLock 
	custLineLock.Release();
	// Acquire the orderTakerLock to wake up the Order Taker who is waiting
	// on the signal from the current customer
	internalOT = custData[myIndex].myOT;
	orderTakerLock[internalOT]->Acquire();

	// Send a signal to the Order Taker which indicates that the customer
	// is ready to give the order
	orderTakerCV[internalOT]->Signal(orderTakerLock[internalOT]);

    // Customer goes on wait to receive a signal from the Order Taker
	// which indicates Order Taker is ready to take the order
	orderTakerCV[internalOT]->Wait(orderTakerLock[internalOT]);
	
	// Customer received a signal from the Order Taker to place an order
	// Order is randomly generated and is stored in the customer database
	
	// OG
	if(managerOT != 1)
	printf("%c  Customer %d is giving order to OrderTaker %d\n",
	4, myIndex, internalOT);
	else
	printf("%c  Customer %d is giving order to the manager\n",
	4, myIndex);
	
	
	if(custData[myIndex].six$Burger == 1)
	printf("%c  Customer %d is ordering 6-dollar burger\n", 4, myIndex);
	else
	printf("%c  Customer %d is not ordering 6-dollar burger\n", 4, myIndex);
	
	if(custData[myIndex].three$Burger == 1)
	printf("%c  Customer %d is ordering 3-dollar burger\n", 4, myIndex);
	else
	printf("%c  Customer %d is not ordering 3-dollar burger\n", 4, myIndex);
	
	if(custData[myIndex].vegBurger == 1)
	printf("%c  Customer %d is ordering veggie burger\n", 4, myIndex);
	else
	printf("%c  Customer %d is not ordering veggie burger\n", 4, myIndex);
	
	if(custData[myIndex].fries == 1)
	printf("%c  Customer %d is ordering french fries\n", 4, myIndex);
	else
	printf("%c  Customer %d is not ordering french fries\n", 4, myIndex);
	
	if(custData[myIndex].soda == 1)
	printf("%c  Customer %d is ordering soda\n", 4, myIndex);
	else
	printf("%c  Customer %d is not ordering soda\n", 4, myIndex);
	
	if(custData[myIndex].dineType == 1)
	printf("%c  Customer %d chooses to to-go the food\n", 4, myIndex);
	else
	printf("%c  Customer %d chooses to eat-in the food\n", 4, myIndex);
	
	// Send a signal to the Order Taker which indicates that the customer
	// has placed an order
	orderTakerCV[internalOT]->Signal(orderTakerLock[internalOT]);
	
	// wait for the Order Taker to reply once he checks the food availability
	orderTakerCV[internalOT]->Wait(orderTakerLock[internalOT]);
	printf("%c  Customer %d wake up after place order and ready to give money\n", 4, myIndex);
	// Received a signal from Order Taker which indicates that the order is 
	// processed and its time to pay money
	
	// Send a signal to the Order Taker which indicates that the customer
	// has payed the bill amount 
	orderTakerCV[internalOT]->Signal(orderTakerLock[internalOT]);
	
	// Wait for the Order Taker to acknowledge the payment being done 
	// successfully
	orderTakerCV[internalOT]->Wait(orderTakerLock[internalOT]);
	
	// Received a signal from the Order Taker to indicate that the payment
	// was completed and the customer can move to the next stage 

	// If to-go then check if the order is bagged right away
	// else go and wait for a broadcast of the tokenNo
	printf("%c  Customer %d is in after confirm payment status\n",4, myIndex);
	
	if(custData[myIndex].dineType == 1){
		// if to-go and if the order is delivered then the customer takes the 
		// bag and just leaves the restaurant
		// release the Order Taker Lock
			orderTakerLock[internalOT]->Release();
		if(custData[myIndex].delivered == 1){
		
			// OG
			if(managerOT != 1)
			printf("%c  Customer %d receives food from the OrderTaker %d\n",4,myIndex, internalOT);
			else
			printf("%c  Customer %d receives food from the Manager %d\n",4,myIndex, internalOT);
			
			
			// Acquire the Lock to update the custer count served the food
			custServedLock.Acquire();
			custServed++;
			printf("\n\nCUSTOMER %d SERVICED IS %d\n\n", myIndex, custServed);
			custServedLock.Release();
			currentThread->Finish();
	
			// if to-go and if the order is not ready then customer has to wait
			// for a broadcast signal from the Order Taker when the order is bagged
		
			// Acquire the Lock which is used to match with To-go waiting
			// condition variable waitingLock
		}else{
		  // OG
		  if(managerOT != 1)
		  printf("%c  TogoCustomer %d is given token number %d by the OrderTaker %d\n",4, myIndex, custData[myIndex].tokenNo, internalOT);
	  	  else
		  printf("%c  Customer %d is given token number %d by the Manager\n",4, myIndex, custData[myIndex].tokenNo);
		
		  waitToGoLock.Acquire();
		
	
		
		  // Go on wait to receive the broadcast signal from the Order Taker
		  //waitToGoCV.Wait(&waitToGoLock);
			
		  toGoCV.Wait(&waitToGoLock);	
		  printf("%c custom %d wake by ordertaker \n", 4, myIndex);
		
		while(1){
		    // Received a Broadcast signal from one of the Order Taker
		    // - Check if the broadcasted tokenNo is customer's tokenNo 
			//		    if(broadcastedTokenNo == custData[myIndex].tokenNo){
			if(custData[myIndex].delivered = 1)	{
				printf("%c validate broadcast tokenNumber for custom %d is %d\n", 4, myIndex, custData[myIndex].tokenNo);
			//				toGoCV.Signal(&waitToGoLock);
			//				custData[myIndex].delivered = 1;
				waitToGoLock.Release();
				
				// Acquire the Lock to update the custer count served the food
				custServedLock.Acquire();
				custServed++;
				printf("\n\nCUSTOMER %d SERVICED IS %d\n\n", myIndex,custServed);
				custServedLock.Release();
				
				currentThread->Finish();
				
				break;
			}else{ //if(broadcastedTokenNo != custData[myIndex].tokenNo)
				// Go on wait to receive the next broadcast signal from the waiter
					printf("%c broadcast tokenNumber for custom %d is not true\n", 4, myIndex);
				toGoCV.Wait(&waitToGoLock);
			//				waitToGoLock.Release();
			}	
		}
	    }
	}	// if dineType is eat-in
	else if(custData[myIndex].dineType == 0){
			orderTakerLock[internalOT]->Release();
		
			// OG
			printf("%c  EatinCustomer %d is given token number %d by the OrderTaker %d\n",
			4, myIndex, custData[myIndex].tokenNo, internalOT);
			//			printf("%c tokenNumber for custom %d is %d\n", 4, myIndex, custData[myIndex].tokenNo);
			// Acquire a Lock to add customer onto the queue waiting for
			// manager's reply
			managerLock.Acquire();
			// Increment the customer waiting count
			managerLineLength++;
			// go on wait till manager signals with table availability
			managerCV.Wait(&managerLock);
			managerLock.Release();
			// Received a signal from manager
			tablesDataLock.Acquire();
			
			// Manager replied saying "restaurant is not full"
			if(tableAvailable == 1){
				
				// OG
				printf("%c  Customer %d is informed by the Manager-the restaurant is not full\n", 4, myIndex);
			
				for(index = 1; index <= TABLE_COUNT; index++){
					if(tables[index] == 0){
						// make the first table which is free as the customer
						// table
						tables[index] = custData[myIndex].tokenNo;
					printf("%c table %d is taken by custom %d tokenNo %d\n", 4, index, myIndex, tables[index]);
						custData[myIndex].tableNo = index;
						break;
					}
				}
				printf("customer found a table index %d\n", index);
				// Release the tables data lock
				tablesDataLock.Release();
				
				// if a table is found then go sit and wait for food
				if(custData[myIndex].tableNo != 0){
				
					// OG	
					printf("%c  customer %d is seated at table number %d\n",4, myIndex, custData[myIndex].tableNo);
					
					
					// Before releasing the tablesDataLock Acquire the 
					// eatInFoodWaitingLock to ensure that this customer is the 
					// next one to join the queue waiting for the food to be bagged
					eatInFoodWaitingLock.Acquire();
					
					
					// customer goes on wait to receive a signal from the waiter
					// after the tokenNo is validated
					while(1){
					eatInFoodWaitingCV.Wait(&eatInFoodWaitingLock);
					
		 			// upon receiving the signal check if order is delivered
					if(custData[myIndex].delivered == 1){
						// if delivered then the customer eats the food and
						// leaves the restaurant
						eatInFoodWaitingLock.Release();
						eatInWaitingLock.Acquire();
						eatInWaitingCV.Signal(&eatInWaitingLock);
						eatInWaitingLock.Release();
						
						
						// Acquire the Lock to update the custer count served the food
						custServedLock.Acquire();
						custServed++;
						printf("\n\nCUSTOMER %d SERVICED IS %d\n\n",myIndex, custServed);
						custServedLock.Release();
						
						currentThread->Finish();
							
					}
					}
				}	// Manager replied saying "restaurant is full"	
			}else if(tableAvailable == 0){
				
				// OG
				printf("%c  Customer %d is informed by the Manager-the restaurant is full\n", 4, myIndex);
				
				// if restaurant is full and no table is available where the
				// customer can sit 
				
				// Acquire the eat-in table waiting lock before releasing the
				// tablesDataLock so that customer wont get context switched 
				// and he can be the next person to join the waiting queue

				
				// Release the tables Data lock
				tablesDataLock.Release();
				eatInWaitingLock.Acquire();
				
				
				// OG
				printf("%c  customer %d is waiting to sit on the table\n",
				4, myIndex);
				
				
				// Customer goes on wait till he receives a signal from
				// a seated customer who received his bag and is leaving
				// the restaurant
				eatInWaitingCV.Wait(&eatInWaitingLock);
				eatInWaitingLock.Release();
				// Received a signal from one of the customer leaving.
				// Acquire the Lock on the tables data to make the 
				// freed table number as the current customer table
				tablesDataLock.Acquire();	
				
				for(index = 1; index <= TABLE_COUNT; index++){
					if(tables[index] == 0){
						// make the first table which is free as the customer
						// table
						tables[index] = custData[myIndex].tokenNo;
						printf("%c table %d is taken by custom %d tokenNo %d\n", 4, index, myIndex, custData[myIndex].tokenNo);
						custData[myIndex].tableNo = index;
					}
					break;
				}
				
				// Release the tables data lock
				tablesDataLock.Release();
				if(custData[myIndex].tableNo != 0){
				//					// found a table so release the eatInWaiting Lock
				//					eatInWaitingLock.Release();
				
					// Before releasing the tablesDataLock Acquire the 
					// eatInFoodWaitingLock to ensure that this customer is the 
					// next one to join the queue waiting for the food to be bagged
					eatInFoodWaitingLock.Acquire();
					
					
					// customer goes on wait to receive a signal from the waiter
					// after the tokenNo is validated
					
					// OG
					printf("%c  Customer %d is waiting for the waiter to serve the food\n",4, myIndex);
					
					eatInFoodWaitingCV.Wait(&eatInFoodWaitingLock);
						
					// upon receiving the signal check if order is delivered
					while(custData[myIndex].delivered != 1){
						// else Go Back to wait
						eatInFoodWaitingCV.Wait(&eatInFoodWaitingLock);	
					}
					
					// OG
					printf("%c Customer %d is served by waiter\n",4, myIndex);
					
					// if delivered then the customer eats the food and
					// leaves the restaurant
					
					// OG
					if(managerOT != 1)
					printf("%c  Customer %d is leaving the restaurant after OrderTaker %d packed the food\n",4, myIndex, internalOT);
					else
					printf("%c  Customer %d is leaving the restaurant after the Manager packed the food\n",4, myIndex);
						
					eatInFoodWaitingLock.Release();
					// once the order is delivered, the seated customer before
					// leaving the restaurant signals one of the customers 
					// who is waiting for a table to sit 
					eatInWaitingLock.Acquire();
					eatInWaitingCV.Signal(&eatInWaitingLock);
					eatInWaitingLock.Release();
					// OG
					printf("%c EAT-IN  Customer %d is leaving the restaurant after having food\n",4, myIndex);
				
					 
					// Acquire the Lock to update the custer count served the food
					custServedLock.Acquire();
					custServed++;
					printf("\n\nCUSTOMER %d SERVICED IS %d\n\n", myIndex, custServed);
					custServedLock.Release();
					
					currentThread->Finish();
					
					
				}
			}
	}
}

int BagTheOrders()
{
	int index = 0;	
	int cannotBeBagged = 0;
	
	// if there are no more customers in the customer line
	// then Order Taker starts bagging the orders
	for(index = 1; index <= CUST_COUNT; index++){
		cannotBeBagged = 0;
		// check for all the orders which are not bagged
		//		if(custData[index].ordered == 1){
		  if(custData[index].bagged != 1){
			// Accessing the shared database
			// Acquire the readyFoodDataBaseLock 
			foodReadyDBLock.Acquire();
			// Acquire the foodToBeCookedLock
			foodToBeCookedDBLock.Acquire();
			// if 6$burger is ordered
			if(custData[index].six$Burger == 1){
				//if six$burgers available
				if(foodReadyData.six$Burger != 0){
					//reserve a burger for this customer
					custData[index].six$Burger = 2;
					// if greater than minimum amount of cooked food
					// then bag it, no worries
					if(foodReadyData.six$Burger > MINCOOKEDFOOD){
						foodReadyData.six$Burger--;	
						foodToBeCookedData.six$Burger--;
					}
					// else if it is less than minimum then 
					// bag it and increment the food to be cooked
					if(foodReadyData.six$Burger <= MINCOOKEDFOOD){
						foodReadyData.six$Burger--;	
						// Do not decrement the foodtoBeCookedData because the
						// amount of cooked food is less than minimum
					}
				}
				else if(foodReadyData.six$Burger == 0)
					cannotBeBagged = 1;
			}
			
			// if 3$burger is ordered
			if(custData[index].three$Burger == 1){
				//if three$burgers available
				if(foodReadyData.three$Burger != 0){
					//reserve a burger for this customer
					custData[index].three$Burger = 2;
					// if greater than minimum amount of cooked food
					// then bag it, no worries
					if(foodReadyData.three$Burger > MINCOOKEDFOOD){
						foodReadyData.three$Burger--;	
						foodToBeCookedData.three$Burger--;
					}
					// else if it is less than minimum then 
					// bag it and do not decrement the food to be cooked
					if(foodReadyData.three$Burger <= MINCOOKEDFOOD){
						foodReadyData.three$Burger--;	
						// Do not decrement the foodtoBeCookedData because the
						// amount of cooked food is less than minimum
					}
				}
				else if(foodReadyData.three$Burger == 0)
					cannotBeBagged = 1;
			}
			
			// if vegburger is ordered
			if(custData[index].vegBurger == 1){
				//if vegburgers available
				if(foodReadyData.vegBurger != 0){
					//reserve a burger for this customer
					custData[index].vegBurger = 2;
					// if greater than minimum amount of cooked food
					// then bag it, no worries
					if(foodReadyData.vegBurger > MINCOOKEDFOOD){
						foodReadyData.vegBurger--;	
						foodToBeCookedData.vegBurger--;
					}
					// else if it is less than minimum then 
					// bag it and do not decrement the food to be cooked
					if(foodReadyData.vegBurger <= MINCOOKEDFOOD){
						foodReadyData.vegBurger--;	
						// Do not decrement the foodtoBeCookedData because the
						// amount of cooked food is less than minimum
					}
				}
				else if(foodReadyData.vegBurger == 0)
					cannotBeBagged = 1;
			}
			
			// if fries is ordered
			if(custData[index].fries == 1){
				//if fries available
				if(foodReadyData.fries != 0){
					//reserve a fries for this customer
					custData[index].fries = 2;
					// if greater than minimum amount of cooked food
					// then bag it, no worries
					if(foodReadyData.fries > MINCOOKEDFOOD){
						foodReadyData.fries--;	
						foodToBeCookedData.fries--;
					}
					// else if it is less than minimum then 
					// bag it and do not decrement the food to be cooked
					if(foodReadyData.fries <= MINCOOKEDFOOD){
						foodReadyData.fries--;	
						// Do not decrement the foodtoBeCookedData because the
						// amount of cooked food is less than minimum
					}
				}
				else if(foodReadyData.fries == 0)
					cannotBeBagged = 1;
			}
			
			// Accessing the shared database
			// Acquire the readyFoodDataBaseLock 
			foodReadyDBLock.Release();
			// Acquire the foodToBeCookedLock
			foodToBeCookedDBLock.Release();
			// If any of the items was not able to be bagged
			// because it was not available
			if(cannotBeBagged == 1){
				// continue with the next customer order
				//				printf("%c inside bag Fuction custom %d cannotbebagged\n",4, index);
			continue;
			}else if(cannotBeBagged == 0){					
				// Order is bagged so update the food to bag count
				foodToBag-- ;
				// make the customer order status as bagged
				custData[index].bagged = 1;
				// Release the foodToBagLock after updating the foodToBag count
				//				foodToBagLock.Release();
				// Order Taker will bag only one order at a time so break
				//				printf("%c inside bag Fuction bagged for custom %d\n", 4, index);
				return (index);
			}
		}		

	}	
	return -1;
}


void OrderTaker(int myIndex)
{
	int index = 0;				// index for looping through customer database
	int custIndex = 0;			// to store the cust index who being served 
	int custHasToWait = 0;		// if to-go and food not ready set this flag
	int billAmount = 0;			// to store the bill amount payed by customer

	while(1)
	{
		// Acquire custLineLock to access the custLineLength monitor variable
		custLineLock.Acquire();
		// Check for any customers waiting in the customer waiting line
		
		// Acquire the foodToBagLock to check if there are any orders to bag
		foodToBagLock.Acquire();
		
		if(custLineLength > 0){
			//			printf("%c the custline is greater than 0\n",4);
		
			// customers are available so I am releasing the food to bag lock
			foodToBagLock.Release();
			// There are customers in the customer waiting line
			// Signal a customer which indicates the customer to come out of
			// the waiting line and give his order
			custLineCV.Signal(&custLineLock);
			
			// Decrement the custLineLength moniter variable value by 1
			custLineLength--;
			
			// Before releasing the custLineLock, 
			// Order Taker changes his status to WAITING
			orderTakerStatus[myIndex] = OT_WAIT;
			custLineLock.Release();
		}		
		else if(foodToBag != 0){
			// If No customers in the customer waiting line and
			// if there is some food to bag
			custLineLock.Release();
			// bag 1 at a time
			// index of the customer whose order was bagged is returned
			index = BagTheOrders();
			
			foodToBagLock.Release();
			
				//			printf("%c outside bag Function custom index is %d\n", 4, index);
				
			if(index > 0){
				
				// OG
				printf("%c  OrderTaker %d packed the food for Customer %d\n",
				4, myIndex, index);
				
				// after bagging an order 
				// if the customer has chosen to-go then broadcast the signal
				// for the customers waiting in the toGoWaiting line
				if(custData[index].dineType == 1){
					// Acquire the Lock which is used to match with To-go
					// waiting condition variable waitingLock
	
					waitToGoLock.Acquire();
					toGoCV.Broadcast(&waitToGoLock);
				
					// set the broadcastedTokenNo monitor variable to the
					// tokenNo of the customer whose order was bagged
					
					foodToBagLock.Acquire();
					if(custData[index].bagged == 1)
						custData[index].delivered = 1;
					
					//					broadcastedTokenNo = custData[index].tokenNo;	
					// Order Taker Broadcasts the signal to all to-go waiting
					// customers after setting the monitor to tokenNo
													
					printf("%c  OrderTaker %d broadcast the token number %d\n",
				    4, myIndex, custData[index].tokenNo);
		
					foodToBagLock.Release();
						
					//					toGoCV.Wait(&waitToGoLock);
						// Release the waiting line Lock so that the customers
						// who received the signal can acquire the Lock and compare
						// their tokenNo with the broadcastedTokenNo
					//					broadcastTokenLock.Release();
					waitToGoLock.Release();	
					
				}// else if the customer has chosen to eat-in then the tokenNo 
				// of the order which is bagged is added to the baggedList 
				// from which the waiter will remove the tokenNo and 
				// deliver it to the eat-in seated customer 
				else if(custData[index].dineType == 0){
					// Acquire the Lock to Append the tokenNo to the 
					// bagged List	
					foodBaggedListLock.Acquire();
					for(int i = 1;i <= CUST_COUNT; i++){
						if(baggedList[i] == 0){
							baggedList[i] = custData[index].tokenNo;
							break;
						}
					}
					
					// OG
					printf("%c ordertaker %d gives Token number %d to Waiter for Customer %d\n",
					4, myIndex, custData[index].tokenNo, index);
					
					foodBaggedListLock.Release();
				
				}
				
			
			}
				currentThread->Yield();
				currentThread->Yield();
				
		} 
		else{
			// no customers are available and no food to bag so
			// releasing the food to bag lock
			foodToBagLock.Release();
			custLineLock.Release();
			// No customers to serve and no food to bag so the Order Taker 
			// has nothing to do, Set the status of that Order Taker as FREE
			// Check if all the Customers are serviced
			custServedLock.Acquire();
			if(custServed == CUST_COUNT){
				printf("ordertaker is Stopping\n");
				custServedLock.Release();
				currentThread->Finish();
				break;
			}
			custServedLock.Release();
			
			orderTakerStatus[myIndex] = OT_FREE;	
		}
			
		if((orderTakerStatus[myIndex] == OT_WAIT) || (orderTakerStatus[myIndex] == OT_FREE))
		{
			// ----------------------added
			// Acquire the orderTakerLock before releasing the custLineLock 
			// - To ensure that the Customer acquires the orderTakerLock 
			//   after Order Taker does
			orderTakerLock[myIndex]->Acquire();

			//Release the custLineLock 
			// - so that the Customer can access the updated status of the Order Taker
			//			custLineLock.Release();
                 
			// Order Taker goes on wait to receive a signal from the Customer 
			// which indicates Customer is ready to place the order
			orderTakerCV[myIndex]->Wait(orderTakerLock[myIndex]);
		 
			// Send a signal to the Customer which indicates that the Order Taker
			// is ready to take the order
			orderTakerCV[myIndex]->Signal(orderTakerLock[myIndex]);
		
			// Order Taker will go on wait till he receives a signal from the 
			// customer after placing the order
			orderTakerCV[myIndex]->Wait(orderTakerLock[myIndex]);
		
			// Received a signal from the customer after placing an order 
			// Find the customer index once the order is placed
			for( index = 1; index <= CUST_COUNT; index++){
				// find the customer index by finding the customer who has their Order
				// taker index as the current Order taker Index Thread
				if(custData[index].myOT == myIndex){
					if(custData[index].ordered != 1){
						custIndex = index;
						break;
					}
				}
			}
			
			// OG
			printf("%c  OrderTaker %d is taking order of Customer %d\n",
			4, myIndex, custIndex);
			
			// Accessing the shared database
		
			
			// Acquire the foodToBeCookedLock
			foodToBeCookedDBLock.Acquire();
			
			// Acquire the readyFoodDataBaseLock 
			foodReadyDBLock.Acquire();
			
			customerDataLock.Acquire();
			// if to-go customer then  - to check if the food is ready right away
			//						   - if not ready then make customer wait
			//						   - modify the food required monitor variable
			if(custData[custIndex].dineType == 1){
				custHasToWait = 0;
				// if 6$burger is ordered
				if(custData[custIndex].six$Burger == 1){
					// if 6$burger is ready
					if(foodReadyData.six$Burger != 0){
						//reserve a burger for this customer
						custData[custIndex].six$Burger = 2;
						// if greater than minimum amount of cooked food
						// then bag it, no worries
						if(foodReadyData.six$Burger > MINCOOKEDFOOD){
							foodReadyData.six$Burger--;	
							foodToBeCookedData.six$Burger--;
						}
						// else if it is less than minimum then 
						// bag it and increment the food to be cooked
						if(foodReadyData.six$Burger <= MINCOOKEDFOOD){
							foodReadyData.six$Burger--;	
							// Do not decrement the foodtoBeCookedData because the
							// amount of cooked food is less than minimum
						}
					}	
					else // customer has to wait
						custHasToWait = 1;
				}
				
				// if 3$burger is ordered
				if(custData[custIndex].three$Burger == 1){
					// if 3$burger is ready
					if(foodReadyData.three$Burger != 0){
						//reserve a burger for this customer
						custData[custIndex].three$Burger = 2;
						// if greater than minimum amount of cooked food
						// then bag it, no worries
						if(foodReadyData.three$Burger > MINCOOKEDFOOD){
						foodReadyData.three$Burger--;	
						foodToBeCookedData.three$Burger--;
						}
						// else if it is less than minimum then 
						// bag it and do not decrement the food to be cooked
						if(foodReadyData.three$Burger <= MINCOOKEDFOOD){
							foodReadyData.three$Burger--;
							// Do not decrement the foodtoBeCookedData because the
							// amount of cooked food is less than minimum
						}
					}	
					else // customer has to wait
							custHasToWait = 1;
				}
				
				// if vegburger is ordered
				if(custData[custIndex].vegBurger == 1){
					// if vegburger is ready
					if(foodReadyData.vegBurger != 0){
						//reserve a burger for this customer
						custData[custIndex].vegBurger = 2;
						// if greater than minimum amount of cooked food
						// then bag it, no worries
						if(foodReadyData.vegBurger > MINCOOKEDFOOD){
							foodReadyData.vegBurger--;	
							foodToBeCookedData.vegBurger--;
						}
						// else if it is less than minimum then 
						// bag it and do not decrement the food to be cooked
						if(foodReadyData.vegBurger <= MINCOOKEDFOOD){
							foodReadyData.vegBurger--;	
							// Do not decrement the foodtoBeCookedData because the
							// amount of cooked food is less than minimum
						}	
					}
					else // customer has to wait
						custHasToWait = 1;
				}
				
				// if fries is ordered
				if(custData[custIndex].fries == 1){
					// if fries is ready
					if(foodReadyData.fries != 0){
						//reserve a fries for this customer
						custData[custIndex].fries = 2;
						// if greater than minimum amount of cooked food
						// then bag it, no worries
						if(foodReadyData.fries > MINCOOKEDFOOD){
							foodReadyData.fries--;	
							foodToBeCookedData.fries--;
						}
						// else if it is less than minimum then 
						// bag it and do not decrement the food to be cooked
						if(foodReadyData.fries <= MINCOOKEDFOOD){
							foodReadyData.fries--;	
							// Do not decrement the foodtoBeCookedData because the
							// amount of cooked food is less than minimum
						}		
					}
					else // customer has to wait
						custHasToWait = 1;
				}
			
				// if to-go and if customer has to wait
				if(custHasToWait == 1){
					// set the token number for the customer
					// Acquire the lock to get the next token number to be given 
					// to the customer, all Order Takers access this value
					// - increment this monitor variable to generate the new unique
					//   token number and give it to customer
					nextTokenNumberLock.Acquire();
					nextTokenNumber++;
					custData[custIndex].tokenNo = nextTokenNumber;
					// Release the lock after obtaining the new token number
					
					nextTokenNumberLock.Release();
					// OG

					printf("%c  OrderTaker %d gives token number %d to Customer %d\n",
					4, myIndex, custData[custIndex].tokenNo, custIndex);
					
				}else{
					// food is ready and can be bagged
					// Even if the customer has ordered only soda
					// it is bagged here
					// Order is bagged so update the food to bag count
					foodToBagLock.Acquire();
					
					// make the customer order status as bagged
					custData[custIndex].bagged = 1;
					custData[custIndex].delivered = 1;
					
					// OG
					printf("%c  OrderTaker %d gives food to Customer %d\n",
					4, myIndex, custIndex);
					
					// Release the foodToBagLock after updating the foodToBag count
					foodToBagLock.Release();
					// Order Taker will bag only one order at a time so break
				}
			}	// dineType = to-go	
				
			// if eat-in
			if(custData[custIndex].dineType == 0){
				custHasToWait = 0;
				// always customer has to wait, he will not get food right away
				
				// set the token number for the customer
				// Acquire the lock to get the next token number to be given 
				// to the customer, all Order Takers access this value
				// - increment this monitor variable to generate the new unique
				//   token number and give it to customer
				nextTokenNumberLock.Acquire();
				nextTokenNumber++;
				custData[custIndex].tokenNo = nextTokenNumber;
				// Release the lock after obtaining the new token number
				// OG
				printf("%c  OrderTaker %d gives token number %d to Customer %d\n",
				4, myIndex, nextTokenNumber, custIndex);
				
				nextTokenNumberLock.Release();
				

				// eat-in customer always waits
				custHasToWait = 1;
				
			}	// dineType = eat-in
			
			// to-go/eat-in 
			if(custHasToWait == 1){
			
				// Acquire the foodToBagLock to increment the foodToBag count
				foodToBagLock.Acquire();
				
				foodToBag++;
				
				// Release the foodToBagLock after updating the foodToBag count
				foodToBagLock.Release();
				
				// Order taker has to update the foodToBeCooked & foodReady database
				// if 6$burger is ordered
				if(custData[custIndex].six$Burger == 1){
					// if 6$burger is ready
					if(foodReadyData.six$Burger != 0){
						//reserve a burger for this customer
						custData[custIndex].six$Burger = 2;
						// if greater than minimum amount of cooked food
						// then bag it, no worries
						if(foodReadyData.six$Burger > MINCOOKEDFOOD){
							foodReadyData.six$Burger--;	
							foodToBeCookedData.six$Burger--;
						}
						// else if it is less than minimum then 
						// bag it and increment the food to be cooked
						if(foodReadyData.six$Burger <= MINCOOKEDFOOD){
							foodReadyData.six$Burger--;	
							// Do not decrement the foodtoBeCookedData because the
							// amount of cooked food is less than minimum
						}
					}
					else // update the requirement in food to be cooked database
						foodToBeCookedData.six$Burger++;
				}
				// if 3$burger is ordered
				if(custData[custIndex].three$Burger == 1){
					// if 3$burger is ready
					if(foodReadyData.three$Burger != 0){
						//reserve a burger for this customer
						custData[custIndex].vegBurger = 2;
						// if greater than minimum amount of cooked food
						// then bag it, no worries
						if(foodReadyData.vegBurger > MINCOOKEDFOOD){
						foodReadyData.vegBurger--;	
						foodToBeCookedData.vegBurger--;
						}
						// else if it is less than minimum then 
						// bag it and do not decrement the food to be cooked
						if(foodReadyData.vegBurger <= MINCOOKEDFOOD){
							foodReadyData.vegBurger--;	
							// Do not decrement the foodtoBeCookedData because the
							// amount of cooked food is less than minimum
						}
					}
					else  // update the requirement in food to be cooked database
						foodToBeCookedData.three$Burger++;
				}
				// if vegburger is ordered
				if(custData[custIndex].vegBurger == 1){
					// if vegburger is ready
					if(foodReadyData.vegBurger != 0){
						//reserve a burger for this customer
						custData[custIndex].vegBurger = 2;
						// if greater than minimum amount of cooked food
						// then bag it, no worries
						if(foodReadyData.vegBurger > MINCOOKEDFOOD){
							foodReadyData.vegBurger--;	
							foodToBeCookedData.vegBurger--;
						}
						// else if it is less than minimum then 
						// bag it and do not decrement the food to be cooked
						if(foodReadyData.vegBurger <= MINCOOKEDFOOD){
							foodReadyData.vegBurger--;	
							// Do not decrement the foodtoBeCookedData because the
							// amount of cooked food is less than minimum
						}	
					}
					else  // update the requirement in food to be cooked database
						foodToBeCookedData.vegBurger++;
				}
				// if fries is ordered
				if(custData[custIndex].fries == 1){
					// if fries is ready
					if(foodReadyData.fries != 0){
						//reserve a fries for this customer
						custData[custIndex].fries = 2;
						// if greater than minimum amount of cooked food
						// then bag it, no worries
						if(foodReadyData.fries > MINCOOKEDFOOD){
							foodReadyData.fries--;	
							foodToBeCookedData.fries--;
						}
						// else if it is less than minimum then 
						// bag it and do not decrement the food to be cooked
						if(foodReadyData.fries <= MINCOOKEDFOOD){
							foodReadyData.fries--;	
							// Do not decrement the foodtoBeCookedData because the
							// amount of cooked food is less than minimum
						}		
					}
					else  // update the requirement in food to be cooked database
						foodToBeCookedData.fries++;
				}
			}
				custData[custIndex].ordered = 1;
			
			
				customerDataLock.Release();
			
				// Releasing the readyFoodDataBaseLock 
				foodReadyDBLock.Release();
				// Releasing the foodToBeCookedLock
				foodToBeCookedDBLock.Release();
				
				// Send a signal to the Customer which indicates that the 
				// Order Taker has processed the order
				orderTakerCV[myIndex]->Signal(orderTakerLock[myIndex]);
				
				// wait for the customer to pay money
				orderTakerCV[myIndex]->Wait(orderTakerLock[myIndex]);
				
				// received signal from customer which indicates that the customer
				// has paid the money
				// Time to put the money received in a place where all money is
				// stored by all the Order takers. This money will be used by the 
				// manager to refill the inventory
				if(custData[custIndex].six$Burger > 0)
					billAmount += 6;	// 6$Burger
				if(custData[custIndex].three$Burger > 0)
					billAmount += 3;	// 3$Burger
				if(custData[custIndex].vegBurger > 0)
					billAmount += 8;	// vegBurger costs 8$	
				if(custData[custIndex].fries > 0)
					billAmount += 2;	// fries costs 2$	
				if(custData[custIndex].soda > 0)
					billAmount += 1;	// soda costs 1$	
				moneyAtRestaurantLock.Acquire();
				moneyAtRestaurant += billAmount;
				// release the lock after storing the money
				moneyAtRestaurantLock.Release();
				
				// Send a signal to the Customer which indicates that the 
				// Order Taker has processed the order
				orderTakerCV[myIndex]->Signal(orderTakerLock[myIndex]);
				
				// interaction with the current customer is completed so the 
				// Order Taker releases the Lock
				orderTakerLock[myIndex]->Release();
				
	    }		
	}  // While(1) 
}	


void Waiter(int myIndex)
{
	int index = 0;
	int tableNo = 0;
	int baggedListNotEmpty = 0;
	int custHasNoTable = 0;
	
	
	while(1){
		
		custHasNoTable = 0;
		index = 0;
		tableNo = 0;
		baggedListNotEmpty = 0;
		// Acquire Lock to check if any orders are bagged
		
		foodBaggedListLock.Acquire();
		// check if token numbers of bagged food are present in bagged list
		
		for(int i = 1;i <= CUST_COUNT; i++){
			if(baggedList[i] != 0){
				baggedListNotEmpty = baggedList[i];
				baggedList[i] = 0;
				break;
			}
		}
		foodBaggedListLock.Release();
		if(baggedListNotEmpty != 0){
			int firstTokenNo;
			
			// remove first token number from the bagged list 
			// to serve it for the customer waiting for it
			firstTokenNo = baggedListNotEmpty;
			// Acquire tableDataLock to obtain the table number of the 
			// Eat-in seated customer 
			printf("%c  %s got the firsttoken number %d \n",
						4, (char*)currentThread->getName(), firstTokenNo);
						
			tablesDataLock.Acquire();
	
			// Determine the table number of the Eat-in Customer
			for(index = 1; index <= TABLE_COUNT; index++){
				// Find the table number using token number removed from 
				// bagged list
				if(tables[index] == firstTokenNo){
					tableNo = index;
				    break;
				}
			}
			printf("%c  %s use the firsttoken number find the tableNo %d \n",
						4, (char*)currentThread->getName(), tableNo);
			
			//			printf("%c %s in tableDataLock %d\n", 4, (char*)currentThread->getName(), index);
			tablesDataLock.Release();
			
			// Search for a particular token number with all customers 
			for(index = 1; index <= CUST_COUNT; index++){
				// If token number of customer matches set the delivered which
				// indicates the food has been served.
				if(custData[index].tokenNo == firstTokenNo){
				printf("%c  %s find custom %d have the same tokenNo %d\n",
						4, (char*)currentThread->getName(), index, firstTokenNo);
						
					// check if the customer is seated
					if(custData[index].tableNo != 0){ 
					
						// OG
						printf("%c  %s validates the token number for Customer %d and serves the food\n",
						4, (char*)currentThread->getName(), index);
						
						custData[index].delivered = 1; 
						
						// OG
					    //						printf("%c  %s serves food to Customer %d\n", 
					    //						4, (char*)currentThread->getName(), index);
					
						// Notifying the Eatin seated customer that the order 
						// has been delivered
						eatInFoodWaitingLock.Acquire(); 
						// Broadcasting the notification about the order
						// delivered. This signal will be processed only by
						// that customer whose order was bagged, others
						// will go back to wait again.
						eatInFoodWaitingCV.Broadcast(&eatInFoodWaitingLock);
						
						eatInFoodWaitingLock.Release(); 
					}
					// else if the customer has not yet got his table to sit
					// then put back the bagged order into the List
					else{ 
						foodBaggedListLock.Acquire();
						for(int i = 1;i <= CUST_COUNT; i++){
							if(baggedList[i] == 0){
								baggedList[i] = firstTokenNo;
								custHasNoTable = 1;
								printf("%c %s find customer %d is not get table\n", 4, (char*)currentThread->getName(), index);
								break;
							
							}
						}
						
						foodBaggedListLock.Release();
					}	
					if(custHasNoTable == 0)
						break;
				}
			}
		}
		if((baggedListNotEmpty == 0) || (custHasNoTable == 1)){
			// Waiter Acquires Lock and Goes on wait (break) till he receives
			// a wake up signal from the manager
			waiterSleepLock.Acquire();
			
			// OG
			printf("%c  %s is going on break\n", 
			4, (char*)currentThread->getName());
			
			waiterSleepCV.Wait(&waiterSleepLock);
			// Received a signal from the manager, waiter starts with his job
			
			waiterSleepLock.Release();
			
			// Check if all the Customers are serviced
			custServedLock.Acquire();
			if(custServed == CUST_COUNT){
				printf("waiter is saying Stopping\n");
				custServedLock.Release();
				currentThread->Finish();
				break;
			}
			custServedLock.Release();
			
			// OG
			printf("%c  %s returned from break\n", 
			4, (char*)currentThread->getName());	
		}
	}
}


void Cook(int whatToCook)
{	
				
	// Acquire a lock on inventory to check status
	 
	while(1){	
		
		// Check if the stop cooking flag is set by the manager
		// Acquire the current cooking item lock
		// Check if the stop cooking flag is set
		// if set then it indicates manager wants the Cook to go on break
		// Add the current cook thread to the cooks on break queue
		// Cooks goes on break (sleep)
		
		
		if(whatToCook == SIX$BURGER){
			stopSix$BurgerLock.Acquire();
			if(stopSix$Burger == 1){
			
				cooksOnBreak->Append((void *)currentThread);
				workingCooks[whatToCook]--;
				// OG
				printf("%c cook %s for type %d is going on break\n", 4,
				currentThread->getName(),whatToCook );
				
				stopSix$BurgerLock.Release();
				wakeUpCookLock.Acquire();
				
				wakeUpCookCV.Wait(&wakeUpCookLock);
				wakeUpCookLock.Release();

				// cook is back from break
				// Acquire the what to cook next lock and check 
				// what to cook next
				whatToCookNextLock.Acquire();
				// update what the cook was cooking before going to 
				// sleep with what he has to cook after the break
				whatToCook = whatToCookNext;
				whatToCookNext = 0;
				
				// release the lock after setting the value
				whatToCookNextLock.Release();
				
				// OG
				printf("%c cook %s for type %d returned from break\n", 4,
				currentThread->getName(), whatToCook);
				
				
			}else{	
				stopSix$BurgerLock.Release();
			}	
		}
		else if(whatToCook == THREE$BURGER){
			stopThree$BurgerLock.Acquire();
			if(stopThree$Burger == 1){
				cooksOnBreak->Append((void *)currentThread);
				workingCooks[whatToCook]--;
				
				
				stopThree$BurgerLock.Release();
				// OG
				printf("%c cook %s for type %d is going on break\n", 4,
				currentThread->getName(),whatToCook );
				
				wakeUpCookLock.Acquire();
			
				wakeUpCookCV.Wait(&wakeUpCookLock);
				wakeUpCookLock.Release();
				
				// cook is back from break
				// Acquire the what to cook next lock and check 
				// what to cook next
				whatToCookNextLock.Acquire();
				// update what the cook was cooking before going to 
				// sleep with what he has to cook after the break
				whatToCook = whatToCookNext;
				whatToCookNext = 0;
				
				// release the lock after setting the value
				whatToCookNextLock.Release();
				
				// OG
				printf("%c cook %s for type %d returned from break\n", 4,
				currentThread->getName(), whatToCook);
				
				
			}else{	
				stopThree$BurgerLock.Release();
			}	
		}
		else if(whatToCook == VEGBURGER){
			stopVegBurgerLock.Acquire();
			if(stopVegBurger == 1){
				cooksOnBreak->Append((void *)currentThread);
				workingCooks[whatToCook]--;
				
				stopVegBurgerLock.Release();
				// OG
				printf("%c cook %s for type %d is going on break\n", 4,
				currentThread->getName(),whatToCook );
				
				wakeUpCookLock.Acquire();
				
				wakeUpCookCV.Wait(&wakeUpCookLock);
				wakeUpCookLock.Release();
				
				// cook is back from break
				// Acquire the what to cook next lock and check 
				// what to cook next
				whatToCookNextLock.Acquire();
				// update what the cook was cooking before going to 
				// sleep with what he has to cook after the break
				whatToCook = whatToCookNext;
				whatToCookNext = 0;
				
				// release the lock after setting the value
				whatToCookNextLock.Release();
				
				// OG
				printf("%c cook %s for type %d returned from break\n", 4,
				currentThread->getName(), whatToCook);
				
				
			}else{
				stopVegBurgerLock.Release();
			}	
		}
		else if(whatToCook == FRIES){
			stopFriesLock.Acquire();
			if(stopFries == 1){
				cooksOnBreak->Append((void *)currentThread);
				
				workingCooks[whatToCook]--;
				stopFriesLock.Release();
				// OG
				printf("%c cook %s for type %d is going on break\n", 4,
				currentThread->getName(),whatToCook );
				
				wakeUpCookLock.Acquire();
				
				wakeUpCookCV.Wait(&wakeUpCookLock);
				wakeUpCookLock.Release();
				
				// cook is back from break
				// Acquire the what to cook next lock and check 
				// what to cook next
				whatToCookNextLock.Acquire();
				// update what the cook was cooking before going to 
				// sleep with what he has to cook after the break
				whatToCook = whatToCookNext;
				whatToCookNext = 0;
				
				// release the lock after setting the value
				whatToCookNextLock.Release();
				
				// OG
				printf("%c cook %s for type %d returned from break\n", 4,
				currentThread->getName(), whatToCook);
				
				
			}else{
				stopFriesLock.Release();
			}	
		} 
		
		// If all the customers are serviced then the cooks will Go Home
		// Check if all the Customers are serviced
		custServedLock.Acquire();
		if(custServed == CUST_COUNT){
			printf("cook is saying Stopping\n");
			custServedLock.Release();
			currentThread->Finish();
			break;
		}
		custServedLock.Release();
		
		// decrement one from the inventory
		inventoryLock.Acquire();
		if(inventory > 0)
			inventory--;
		else
			printf("%c inventory is empty informed by %s", 4, 
		currentThread->getName());
		
	    inventoryLock.Release();
		//		currentThread->Yield();
		// Acquire the food ready DB Lock to update the ready food quantity
		foodReadyDBLock.Acquire();
		
		// check what Manager has ordered the cook to prepare.
		// Increase the food count in Ready DataBase
		if(whatToCook == SIX$BURGER){
		
		// OG
		printf("%c Cook %s is going to cook 6-dollar burger\n", 4, 
		currentThread->getName());
		
			foodReadyData.six$Burger++; 
		}	
		if(whatToCook == THREE$BURGER){
		
		// OG
		printf("%c Cook %s is going to cook 3-dollar burger\n", 4, 
		currentThread->getName());
		
			foodReadyData.three$Burger++;		
		}
		if(whatToCook == VEGBURGER){
		
		// OG
		printf("%c Cook %s is going to cook veggie burger\n", 4, 
		currentThread->getName());	
		
			foodReadyData.vegBurger++; 
		}	
		if(whatToCook == FRIES){
		// OG
		printf("%c Cook %s is going to cook french fries\n", 4, 
		currentThread->getName());	
			foodReadyData.fries++;  
		}	
		
		// release the lock after updating the food that is cooked	
		foodReadyDBLock.Release();
		
		// cooks takes some Time to prepare the food that 
		// he is instructed to cook
		currentThread->Yield();
		currentThread->Yield();
	}
} 

void Manager()
{
	int inventoryRefillCost = 50;
	int timeToGoToBank = 10;
	int tablesFree = 0;
	int foodRequired[5];
	Thread *t, *thread;
	int cookCount = 0;
	char *name_cook1;
	char *name_cook2;
	char *name = new char[20];
	for(int i = 0; i < 6; i++)
	foodRequired[i] = 0;
	int index = 0;
	//	int workingCooks[5];
	cooksOnBreak = new List;
	int baggedListNotEmpty = 0;


	
    while(1){
		// Check if all the Customers are serviced
		custServedLock.Acquire();
		if(custServed == CUST_COUNT){
			printf("Manager is saying Stopping\n");
			custServedLock.Release();
			
			// Waking up all the waiters and telling them to go Home
			waiterSleepLock.Acquire();
			waiterSleepCV.Broadcast(&waiterSleepLock);
			waiterSleepLock.Release();
			
			// Waking up all the Cooks and telling them to go Home
			wakeUpCookLock.Acquire();
			wakeUpCookCV.Broadcast(&wakeUpCookLock);
			wakeUpCookLock.Release();
				
			// Signal all the ordertakers and tell them to go home
			for(index = 1; index < OT_COUNT; index++)
			{
				orderTakerLock[index]->Acquire();
				orderTakerCV[index]->Signal(orderTakerLock[index]);
				orderTakerLock[index]->Release();
			}
			printf("\n\tCARL'S JR RESTAURANT SIMULATION COMPLETED SUCCESSFULLY\n");
			currentThread->Finish();
			break;
		}
		custServedLock.Release();
		
		tablesFree = 0;
		// Acquire the Inventory Lock 
		inventoryLock.Acquire();
	
		// Ckeck the inventory Level
		if(inventory <= MININVENTORY){
			// take a Lock to access the common location where all the money
			// is stored by the OrderTakers
			printf("%c  Manager find inventory is empty\n", 4);
			moneyAtRestaurantLock.Acquire();
		
			if(moneyAtRestaurant < inventoryRefillCost){
				// Take all the money in the restaurant
				moneyAtRestaurant = moneyAtRestaurant + 100;
				// Manager goes to bank to withdraw the remaining money
				// This process of going to bank and withdrawing will take
				// a minimum of five times the time 
				
				// OG
				printf("%c  Manager goes to bank to withdraw the cash\n", 4);	
				
				for(index = 1; index <= timeToGoToBank; index++){
					currentThread->Yield();
				}								
				// OG
				printf("%c  moneyAtRestautant is loaded in the restaurant\n", 4);
				
			}			
			moneyAtRestaurant = moneyAtRestaurant - inventoryRefillCost;									
			// Once He comes Back from the Bank The Inventory will be filled
				inventory = FILLED;
			// OG
			printf("%c  Manager refills the inventory\n", 4);
			moneyAtRestaurantLock.Release();
		}
		inventoryLock.Release();
		
		// Manager Interaction With The Customer
			managerLock.Acquire();
			tableAvailable = 0;
			if(managerLineLength != 0){
			managerLock.Release();	
				// Acquire the tables data lock to check the tables free
				tablesDataLock.Acquire();
				// Find the Number of Tables available free
				for(index = 1; index <= TABLE_COUNT; index++)
					if(tables[index] == 0){
					tablesFree = 1;
					break;
				}
				
				if(tablesFree == 1){
					tableAvailable = 1;
					tablesDataLock.Release();
					managerLock.Acquire();
					managerCV.Signal(&managerLock);
				}else{
					tablesDataLock.Release();
					tableAvailable = 0;
					managerLock.Acquire();
					managerCV.Signal(&managerLock);
				}
				
			}
			managerLock.Release();		
					// ManagerLineLength != 0		
		/* else if(managerLineLength == 0){
			managerLock.Release();
		} */
		// Manager interaction with the cook
		
		// Acquire the ready food and food to be cooked locks so that the manager
		// can decide whether to make a cook go on break or bring back a cook on
		// break or to hire a new cook
		foodToBeCookedDBLock.Acquire();
		foodReadyDBLock.Acquire();
		
		// Find the amount of each type of food still yet to be cooked 
		foodRequired[SIX$BURGER] = (foodToBeCookedData.six$Burger - (foodReadyData.six$Burger - MINCOOKEDFOOD));
		foodRequired[THREE$BURGER] = (foodToBeCookedData.three$Burger - (foodReadyData.three$Burger  - MINCOOKEDFOOD));
		foodRequired[VEGBURGER] = (foodToBeCookedData.vegBurger - (foodReadyData.vegBurger - MINCOOKEDFOOD));
		foodRequired[FRIES] = (foodToBeCookedData.fries - (foodReadyData.fries - MINCOOKEDFOOD));
		
		//		printf("%c foodRequired 6burger %d\n",4,foodRequired[SIX$BURGER]);
		//		printf("%c foodRequired 3burger %d\n",4,foodRequired[THREE$BURGER]);
		//		printf("%c foodRequired viggiebuger %d\n",4,foodRequired[VEGBURGER] );
		//		printf("%c foodRequired fries %d\n",4,foodRequired[FRIES]);
		
		foodToBeCookedDBLock.Release();
		foodReadyDBLock.Release();
		
		// For all the items in the restaurant
		for(index = 1; index < 5; index ++){
		
			// if foodToBeCooked is  a little greater than the foodready
			if((foodRequired[index] <= MAXDIFF) && (foodRequired[index] > 0)){
				// Reset the stop cooking flag for this item if it is set
				if(index == SIX$BURGER){
					stopSix$BurgerLock.Acquire();
					stopSix$Burger = 0;
					stopSix$BurgerLock.Release();
				}
				else if(index == THREE$BURGER){
					stopThree$BurgerLock.Acquire();
					stopThree$Burger = 0;
					stopThree$BurgerLock.Release();
				}
				else if(index == VEGBURGER){
					stopVegBurgerLock.Acquire();
					stopVegBurger = 0;
					stopVegBurgerLock.Release();
				}
				else if(index == FRIES){
					stopFriesLock.Acquire();
					stopFries = 0;
					stopFriesLock.Release();
				}
				
				// if there are no working cooks for this item
				printf("working cook for type %d is %d\n",index,workingCooks[index]);
				
				if(workingCooks[index] == 0){
					// if there are no cooks on break
						if(cooksOnBreak->IsEmpty() == TRUE){
						// Then Create a new Cook
						if(cookCount < COOK_COUNT){
							name_cook1 = new char[20];
							name = "Cook";
							cookCount++;
							sprintf(name_cook1, "%s%d", name, cookCount);
							t = new Thread(name_cook1);
							t->Fork((VoidFunctionPtr)Cook, index);
							workingCooks[index]++;
							printf("create cook %s to cook item %d\n", name_cook1,index);
						    delete[] name_cook1;
						}
						}
						else{
							// bring back the cook on break
							// Acquire the what to cook next lock and tell the cook
							// who is back from the break "what to cook next"
							whatToCookNextLock.Acquire();
							// make what to cook next as the current food item
							while(whatToCookNext != 0){
							whatToCookNextLock.Release();
							currentThread->Yield();
							}
							whatToCookNextLock.Acquire();
							whatToCookNext = index;
							// release the lock after setting the value
							whatToCookNextLock.Release();
							// call the cook on break back to work 
							thread = (Thread *)cooksOnBreak->Remove();
							wakeUpCookLock.Acquire();
							wakeUpCookCV.Signal(&wakeUpCookLock);
							wakeUpCookLock.Release();
							printf("wakeup cook %s to cook item %d\n", name_cook1,index);
							// increment working cooks for this item by 1
							workingCooks[index]++;
						}	
						
				}
			}		
			 // else if the food required is greater than the maximum difference 
			// that can be handled by the current cooks
			else if((foodRequired[index] > MAXDIFF)){
				// Reset the stop cooking flag for this item if it is set
				if(index == SIX$BURGER){
					stopSix$BurgerLock.Acquire();
					stopSix$Burger = 0;
					stopSix$BurgerLock.Release();
				}
				else if(index == THREE$BURGER){
					stopThree$BurgerLock.Acquire();
					stopThree$Burger = 0;
					stopThree$BurgerLock.Release();
				}
				else if(index == VEGBURGER){
					stopVegBurgerLock.Acquire();
					stopVegBurger = 0;
					stopVegBurgerLock.Release();
				}
				else if(index == FRIES){
					stopFriesLock.Acquire();
					stopFries = 0;
					stopFriesLock.Release();
				}
				
				// if no cooks are on break
				if(cooksOnBreak->IsEmpty() == TRUE){
					// if a new cook can be created
					if(cookCount < COOK_COUNT){
						//cookCount = 0;
						// Create a new Cook
						name_cook2 = new char[20];
						name = "Cook";
						cookCount++;
						sprintf(name_cook2, "%s%d", name, cookCount);
						t = new Thread(name_cook2);
						t->Fork((VoidFunctionPtr)Cook, index);
						workingCooks[index]++;
							printf("create cook %s to cook item %d\n", name_cook2, index);
						delete[] name_cook2;	
					}
				}
				else{
					// bring back the cook on break
					// Acquire the what to cook next lock and tell the cook
					// who is back from the break "what to cook next"
					whatToCookNextLock.Acquire();
					// make what to cook next as the current food item
					while(whatToCookNext != 0){
						
						whatToCookNextLock.Release();
						currentThread->Yield();
					}
					
					whatToCookNextLock.Acquire();
					whatToCookNext = index;
					// release the lock after setting the value
					whatToCookNextLock.Release();
					// call the cook on break back to work 
					thread = (Thread *)cooksOnBreak->Remove();
					
					wakeUpCookLock.Acquire();
					wakeUpCookCV.Signal(&wakeUpCookLock);
					wakeUpCookLock.Release();
					printf("wakeup cook %s to cook item %d\n", name_cook1,index);
					// increment working cooks for this item by 1
					workingCooks[index]++;
				}
			}// if the food ready is greater than the required then 
			// set the stop cooking monitor for that item 
			else if((foodRequired[index] < 0)){
				if(index == SIX$BURGER){
					stopSix$BurgerLock.Acquire();
					stopSix$Burger = 1;
				//					workingCooks[index] = 0;
					stopSix$BurgerLock.Release();
				}
				else if(index == THREE$BURGER){
					stopThree$BurgerLock.Acquire();
					stopThree$Burger = 1;
				//					workingCooks[index] = 0;
					stopThree$BurgerLock.Release();
				}
				else if(index == VEGBURGER){
					stopVegBurgerLock.Acquire();
					stopVegBurger = 1;
				//					workingCooks[index] = 0;
					stopVegBurgerLock.Release();
				}
				else if(index == FRIES){
					stopFriesLock.Acquire();
					stopFries = 1;
				//				workingCooks[index] = 0;
					stopFriesLock.Release();
				}
			}
		}// for all the items
	    
		
		//foodToBeCookedDBLock.Release();
		//foodReadyDBLock.Release();
		// Manager working as a Order Taker
		/* 
		// Acquire custLineLock to access the custLineLength monitor variable
		custLineLock.Acquire();
		// Check if the customer line length is 3 times the number of OrderTakers 
		if(custLineLength > (3 * OT_COUNT)){
			// manager will act as a Order Taker and he will service two customers
			//ManagerAsOrderTaker((OT_COUNT + 1));
		}
		custLineLock.Release();
		 */
		// Manager interaction with the waiter
		// If there are some bagged orders on the bagged list
		// then the manager wakes up the waiters
		foodBaggedListLock.Acquire();
		baggedListNotEmpty = 0;
		for(int i = 1;i <= CUST_COUNT; i++){
			if(baggedList[i] != 0){
				baggedListNotEmpty = 1;
				break;
			}
		}
		foodBaggedListLock.Release();
		if(baggedListNotEmpty == 1 ){
			// Manager Acquires Lock and signals the waiters waiting
			waiterSleepLock.Acquire();
			waiterSleepCV.Broadcast(&waiterSleepLock);
			// OG
			printf("%c  Manager calls back all Waiters from break\n", 4);
			waiterSleepLock.Release();
		}	
		currentThread->Yield();
		currentThread->Yield();
		currentThread->Yield();
		
    }// while	
}				
	
// test cases

void testCase1()
{
	int index = 0;
	Thread *t;
	orderTakerStatus = new int[2];
	char* name = new char[20];

	// set the initial status of all the ordertakers as BUSY
	// - So every customer will join the customer queue
	for(index = 1; index <= 2; index++)		 
		orderTakerStatus[index] = OT_BUSY;	

	// Create a condition variable to sequence the actions with Order taker
	for(index = 1; index <= 2; index++){
		name = "orderTakerCV";
		char *name4 = new char[20];
		sprintf(name4, "%s%d", name, index);
		orderTakerCV[index] = new Condition(name4);
	}	
	
	// Append the orderTakerLock name with its index 
	// - to form a unique identifier for every orderTakerLock 
	for(index = 1; index <= 2; index++){	 
		name = new char[15];					       
		name = "orderTakerLock";
        char *name1 = new char[20];		
                 
        sprintf(name1, "%s%d", name, index);
        orderTakerLock[index] = new Lock(name1);
	}
	
	
	// Append the Order Taker with his index
	// - to form a unique identifier for every Order Taker
	for(index = 1; index <= 2; index++){  
	    name = new char[20];					 
		name = "OrderTaker";
	    char *name2 = new char[20];		
                   	
		sprintf(name2, "%s%d", name, index);		 
	    t = new Thread(name2);	
        t->Fork((VoidFunctionPtr)OrderTaker,index);
	}
	
	// Spawning a Manager Thread
        name = new char[20];
        name = "Manager";
		char *managerName = new char[20];
		index = 1;
		sprintf(managerName, "%s%d", name, index);		 
	    t = new Thread(managerName);	
        t->Fork((VoidFunctionPtr)Manager, 0);
	
	// Append the Customer with his index
	// - to form a unique identifier for every Customer
	for(index = 1; index <= 5; index++){  
		name = new char[20];					   
		name = "Customer";
        char *name3 = new char[20];								       
	    //make all customer choose Eat-in
		custData[index].dineType = 0;
        
		sprintf(name3, "%s%d", name, index);		 
        t = new Thread(name3);
		t->Fork((VoidFunctionPtr)Customer,index);		
	}
	
}

void testCase2()
{
	int index = 0;
	Thread *t;
	char* name = new char[20];
	// making all the items in food ready database as 0
	foodReadyData.six$Burger = 0;
	foodReadyData.three$Burger = 0;;
	foodReadyData.vegBurger = 0;
	foodReadyData.fries = 0;
	
	orderTakerStatus = new int[1];

	// set the initial status of all the ordertakers as BUSY
	// - So every customer will join the customer queue
	for(index = 1; index <= 1; index++)		 
		orderTakerStatus[index] = OT_BUSY;	

	// Create a condition variable to sequence the actions with Order taker
	for(index = 1; index <= 1; index++){
		name = "orderTakerCV";
		char *name4 = new char[20];
		sprintf(name4, "%s%d", name, index);
		orderTakerCV[index] = new Condition(name4);
	}	
	
	// Append the orderTakerLock name with its index 
	// - to form a unique identifier for every orderTakerLock 
	for(index = 1; index <= 1; index++){	 
		name = new char[15];					       
		name = "orderTakerLock";
        char *name1 = new char[20];		
                 
        sprintf(name1, "%s%d", name, index);
        orderTakerLock[index] = new Lock(name1);
	}
	
	
	// Append the Order Taker with his index
	// - to form a unique identifier for every Order Taker
	for(index = 1; index <= 1; index++){  
	    name = new char[20];					 
		name = "OrderTaker";
	    char *name2 = new char[20];		
                   	
		sprintf(name2, "%s%d", name, index);		 
	    t = new Thread(name2);	
        t->Fork((VoidFunctionPtr)OrderTaker,index);
	}
	
	// Spawning a Manager Thread
        name = new char[20];
        name = "Manager";
		char *managerName = new char[20];
		index = 1;
		sprintf(managerName, "%s%d", name, index);		 
	    t = new Thread(managerName);	
        t->Fork((VoidFunctionPtr)Manager, 0);
	
	// Append the Customer with his index
	// - to form a unique identifier for every Customer
	for(index = 1; index <= 1; index++){  
		name = new char[20];					   
		name = "Customer";
        char *name3 = new char[20];								       
	    //make all customer choose Eat-in
		custData[index].dineType = 0;
        
		sprintf(name3, "%s%d", name, index);		 
        t = new Thread(name3);
		t->Fork((VoidFunctionPtr)Customer,index);		
	}
}

void testCase3()
{

int index = 0;
Thread *t;
orderTakerStatus = new int[1];
char* name = new char[20];

	// set the initial status of all the ordertakers as BUSY
	// - So every customer will join the customer queue
	for(index = 1; index <= 1; index++)		 
		orderTakerStatus[index] = OT_BUSY;	

	// Create a condition variable to sequence the actions with Order taker
	for(index = 1; index <= 1; index++){
		name = "orderTakerCV";
		char *name4 = new char[20];
		sprintf(name4, "%s%d", name, index);
		orderTakerCV[index] = new Condition(name4);
	}	
	
	// Append the orderTakerLock name with its index 
	// - to form a unique identifier for every orderTakerLock 
	for(index = 1; index <= 1; index++){	 
		name = new char[15];					       
		name = "orderTakerLock";
        char *name1 = new char[20];		
                 
        sprintf(name1, "%s%d", name, index);
        orderTakerLock[index] = new Lock(name1);
	}
	
	
	// Append the Order Taker with his index
	// - to form a unique identifier for every Order Taker
	for(index = 1; index <= 1; index++){  
	    name = new char[20];					 
		name = "OrderTaker";
	    char *name2 = new char[20];		
                   	
		sprintf(name2, "%s%d", name, index);		 
	    t = new Thread(name2);	
        t->Fork((VoidFunctionPtr)OrderTaker,index);
	}
	
	// Spawning a Manager Thread
        name = new char[20];
        name = "Manager";
		char *managerName = new char[20];
		index = 1;
		sprintf(managerName, "%s%d", name, index);		 
	    t = new Thread(managerName);	
        t->Fork((VoidFunctionPtr)Manager, 0);
		
	// Spawning a Waiter Threads
		name = new char[20];
		name = "Waiter";
		char *waiterName = new char[20];
		index = 1;
		sprintf(waiterName, "%s%d", name, index);		 
	    t = new Thread(waiterName);	
        t->Fork((VoidFunctionPtr)Waiter, 0);
		
	
	// Append the Customer with his index
	// - to form a unique identifier for every Customer
	for(index = 1; index <= 2; index++){  
		name = new char[20];					   
		name = "Customer";
        char *name3 = new char[20];								       
		int type = 0;
	    //make all customer choose Eat-in
		custData[index].six$Burger = 1;
		custData[index].vegBurger = 1;
		custData[index].soda = 1;
		custData[index].dineType = type;
        type++; // make the next customer to go
		sprintf(name3, "%s%d", name, index);		 
        t = new Thread(name3);
		t->Fork((VoidFunctionPtr)Customer,index);		
	}



}

void testCase4()
{
	int index = 0;
	Thread *t;
	char* name = new char[20];
	   inventory = 30; // setting inventory below minimum level
	  
	  // Spawning a Manager Thread
        name = new char[20];
        name = "Manager";
		char *managerName = new char[20];
		index = 1;
		sprintf(managerName, "%s%d", name, index);		 
	    t = new Thread(managerName);	
        t->Fork((VoidFunctionPtr)Manager, 0);


}

void testCase5()
{
	int index = 0;
	Thread *t;
	orderTakerStatus = new int[1];
	char* name = new char[20];
	// set the initial status of all the ordertakers as BUSY
	// - So every customer will join the customer queue
	for(index = 1; index <= 1; index++)		 
		orderTakerStatus[index] = OT_BUSY;	

	// Create a condition variable to sequence the actions with Order taker
	for(index = 1; index <= 1; index++){
		name = "orderTakerCV";
		char *name4 = new char[20];
		sprintf(name4, "%s%d", name, index);
		orderTakerCV[index] = new Condition(name4);
	}	
	
	// Append the orderTakerLock name with its index 
	// - to form a unique identifier for every orderTakerLock 
	for(index = 1; index <= 1; index++){	 
		name = new char[15];					       
		name = "orderTakerLock";
        char *name1 = new char[20];		
                 
        sprintf(name1, "%s%d", name, index);
        orderTakerLock[index] = new Lock(name1);
	}
	
	
	// Append the Order Taker with his index
	// - to form a unique identifier for every Order Taker
	for(index = 1; index <= 1; index++){  
	    name = new char[20];					 
		name = "OrderTaker";
	    char *name2 = new char[20];		
                   	
		sprintf(name2, "%s%d", name, index);		 
	    t = new Thread(name2);	
        t->Fork((VoidFunctionPtr)OrderTaker,index);
	}
		
	
	// Append the Customer with his index
	// - to form a unique identifier for every Customer
	for(index = 1; index <= 1; index++){  
		name = new char[20];					   
		name = "Customer";
        char *name3 = new char[20];								       
	
	    //make the customer To-go
	
		custData[index].soda = 1;
		custData[index].dineType = 1;
        
		sprintf(name3, "%s%d", name, index);		 
        t = new Thread(name3);
		t->Fork((VoidFunctionPtr)Customer,index);		
	}

}

void testCase6()
{
int index = 0;
Thread *t;
char* name = new char[20];
orderTakerStatus = new int[1];

	// set the initial status of all the ordertakers as BUSY
	// - So every customer will join the customer queue
	for(index = 1; index <= 1; index++)		 
		orderTakerStatus[index] = OT_BUSY;	

	// Create a condition variable to sequence the actions with Order taker
	for(index = 1; index <= 1; index++){
		name = "orderTakerCV";
		char *name4 = new char[20];
		sprintf(name4, "%s%d", name, index);
		orderTakerCV[index] = new Condition(name4);
	}	
	
	// Append the orderTakerLock name with its index 
	// - to form a unique identifier for every orderTakerLock 
	for(index = 1; index <= 1; index++){	 
		name = new char[15];					       
		name = "orderTakerLock";
        char *name1 = new char[20];		
                 
        sprintf(name1, "%s%d", name, index);
        orderTakerLock[index] = new Lock(name1);
	}
	
	
	// Append the Order Taker with his index
	// - to form a unique identifier for every Order Taker
	for(index = 1; index <= 1; index++){  
	    name = new char[20];					 
		name = "OrderTaker";
	    char *name2 = new char[20];		
                   	
		sprintf(name2, "%s%d", name, index);		 
	    t = new Thread(name2);	
        t->Fork((VoidFunctionPtr)OrderTaker,index);
	}
	
	// Spawning a Manager Thread
        name = new char[20];
        name = "Manager";
		char *managerName = new char[20];
		index = 1;
		sprintf(managerName, "%s%d", name, index);		 
	    t = new Thread(managerName);	
        t->Fork((VoidFunctionPtr)Manager, 0);
		
	// Spawning a Waiter Threads
		name = new char[20];
		name = "Waiter";
		char *waiterName = new char[20];
		index = 1;
		sprintf(waiterName, "%s%d", name, index);		 
	    t = new Thread(waiterName);	
        t->Fork((VoidFunctionPtr)Waiter, 0);
		
	
	// Append the Customer with his index
	// - to form a unique identifier for every Customer
	for(index = 1; index <= 4; index++){  
		name = new char[20];					   
		name = "Customer";
        char *name3 = new char[20];								       
		
	    //make all customer choose Eat-in
		custData[index].six$Burger = 1;
		custData[index].vegBurger = 1;
		custData[index].soda = 1;
		custData[index].dineType = 1;
        
		sprintf(name3, "%s%d", name, index);		 
        t = new Thread(name3);
		t->Fork((VoidFunctionPtr)Customer,index);		
	}
}

void testCase7()
{
	int index = 0;
	Thread *t;
	char* name = new char[20];
	// making money at restaurant 0
		moneyAtRestaurant = 0;
		inventory = 30;
	// Spawning a Manager Thread
        name = new char[20];
        name = "Manager";
		char *managerName = new char[20];
		
		sprintf(managerName, "%s%d", name, index);		 
	    t = new Thread(managerName);	
        t->Fork((VoidFunctionPtr)Manager, 0);

}

void testCase8()
{
	int index = 0;
	Thread *t;
	char* name = new char[20];
	// setting food level in to be cooked database
		foodToBeCookedData.six$Burger = 2;
		
	// setting food level in to be ready database
		foodReadyData.six$Burger = 1;
		
		
	// Spawning a Manager Thread
        name = new char[20];
        name = "Manager";
		char *managerName = new char[20];
		index = 1;
		sprintf(managerName, "%s%d", name, index);		 
	    t = new Thread(managerName);	
        t->Fork((VoidFunctionPtr)Manager, 0);

}


// test cases
void CarlsJr()
{
	Thread *t;
	int index = 0;
	char *name = NULL;
	int ot_count, cust_count, cook_count, waiter_count;
	
	//testsuite();
	// User Input for the number of entities to be present in the simulation
	
	// Receive the number of Order Takers to be present in the simulation
	printf("Enter the Number of OrderTakers in Carls Jr \n");		 
	scanf("%d", &ot_count);
	OT_COUNT = ot_count;
	// Receive the number of Waiters to be present in the simulation
	printf("Enter the Number of Waiters in Carls Jr ");			 
	scanf("%d", &waiter_count);
	WAITER_COUNT = waiter_count;
	// Receive the number of Cooks to be present in the simulation
	printf("Enter the Number of Cooks in Carls Jr ");			 
	scanf("%d", &cook_count);
	COOK_COUNT = cook_count;
	// Receive the number of Customers to be present in the simulation
	printf("Enter the Number of Customers in Carls Jr ");		 
	scanf("%d", &cust_count);
	CUST_COUNT = cust_count; 
	
	//Printing the Initial Values before start of Simulation
	printf("o Number of Ordertakers = %d\n", OT_COUNT);
	printf("o Number of Waiters = %d\n", WAITER_COUNT);
	printf("o Number of Cooks = %d\n", COOK_COUNT);
	printf("o Number of Customers = %d\n", CUST_COUNT);
	printf("\n Restaurant\n");
	printf("\t o Total Number of tables in the Restaurant = %d\n", TABLE_COUNT);
	printf("\t o Minimum Number of cooked 6-dollar burger = %d\n", MINCOOKEDFOOD);
	printf("\t o Minimum Number of cooked 3-dollar burger = %d\n", MINCOOKEDFOOD);
	printf("\t o Minimum Number of cooked veggie burger = %d\n", MINCOOKEDFOOD);
	printf("\t o Minimum Number of cooked french fries = %d\n", MINCOOKEDFOOD);
	
	customerDataLock.Acquire();
	// random order is generated for all the customers
	generateCustomerOrder();
	customerDataLock.Release();	
	
	foodReadyData.six$Burger = MINCOOKEDFOOD; 
	foodReadyData.three$Burger = MINCOOKEDFOOD;
	foodReadyData.vegBurger = MINCOOKEDFOOD;
	foodReadyData.fries = MINCOOKEDFOOD; 
	
	// Find the amount of each type of food still yet to be cooked 
	foodToBeCookedData.six$Burger  = 0;
	foodToBeCookedData.three$Burger = 0;
	foodToBeCookedData.vegBurger  = 0;
	foodToBeCookedData.fries = 0;
	
    orderTakerStatus = new int[OT_COUNT];

	// set the initial status of all the ordertakers as BUSY
	// - So every customer will join the customer queue
	for(index = 1; index <= OT_COUNT; index++)		 
		orderTakerStatus[index] = OT_BUSY;	

	// Create a condition variable to sequence the actions with Order taker
	for(index = 1; index <= OT_COUNT; index++){
		name = "orderTakerCV";
		char *name4 = new char[20];
		sprintf(name4, "%s%d", name, index);
		orderTakerCV[index] = new Condition(name4);
	}	
	
	char *nameManager = new char[20];
	nameManager = "managerOtCV";
	char *name5 = new char[20];
	sprintf(name5, "%s%d", name, (OT_COUNT + 1));
	// Create a Condition variable for manager as a Order taker
	orderTakerCV[(OT_COUNT + 1)] = new Condition(name5);
	
	// Append the orderTakerLock name with its index 
	// - to form a unique identifier for every orderTakerLock 
	for(index = 1; index <= OT_COUNT; index++){	 
		name = new char[15];					       
		name = "orderTakerLock";
        char *name1 = new char[20];		
                 
        sprintf(name1, "%s%d", name, index);
        orderTakerLock[index] = new Lock(name1);
	}
	
	char *nameManagerLock = new char[20];
	nameManagerLock = "managerOtLock";
	// Create a Lock for manager as a Order taker
	orderTakerLock[(OT_COUNT + 1)] = new Lock(nameManagerLock);
	
	// Append the Order Taker with his index
	// - to form a unique identifier for every Order Taker
	for(index = 1; index <= OT_COUNT; index++){  
	    name = new char[20];					 
		name = "OrderTaker";
	    char *name2 = new char[20];		
                   	
		sprintf(name2, "%s%d", name, index);		 
	    t = new Thread(name2);	
        t->Fork((VoidFunctionPtr)OrderTaker,index);
	}
	
	// - to form a unique identifier for every waiter
	for(index = 1; index <= WAITER_COUNT; index++){  
	    name = new char[20];					 
		name = "Waiter";
	    char *name6 = new char[20];		
                   	
		sprintf(name6, "%s%d", name, index);		 
		t = new Thread(name6);	
        t->Fork((VoidFunctionPtr)Waiter, index);
	}
	
	// Spawning a Manager Thread
		char *managerName = new char[20];
	    managerName = "Manager";
		t = new Thread(managerName);	
        t->Fork((VoidFunctionPtr)Manager,0);
	
	// Append the Customer with his index
	// - to form a unique identifier for every Customer
	for(index = 1; index <= CUST_COUNT; index++){  
		name = new char[20];					   
		name = "Customer";
        char *name3 = new char[20];								       
	    	
        sprintf(name3, "%s%d", name, index);		 
        t = new Thread(name3);
		t->Fork((VoidFunctionPtr)Customer, index);		
	}

		
}

void Problem2()
{
   char choice;
   int index = 0, testCaseChoice = 0;
  
   printf(" Do you want to run Test Cases ? y or n");
   scanf("%c",&choice);
   if(choice == 'y'){
   printf(" TEST CASE MENU\n");
   printf(" 1. Customers who wants to eat-in, must wait if the restaurant is full\n");
   printf(" 2. OrderTaker/Manager gives order number to the Customer when the food is not ready\n");
   printf(" 3. Customers who have chosent to eat-in, must leave after they have their food and Customers\n");
   printf("    who have chosen to-go, must leave the restaurant after the OrderTaker/Manager has given the food\n");
   printf(" 4. Manager maintains the track of food inventory. Inventory is refilled when it goes below order level\n");
   printf(" 5. A Customer who orders only soda need not wait\n");
   printf(" 6. The OrderTaker and the Manager both somethimes bag the food\n");
   printf(" 7. Manager goes to the bank for cash when inventory is to be refilled and there is no cash in the restaurant\n");
   printf(" 8. Cooks goes on break when informed by manager\n");
   printf(" 9. Run Carls Junior Restaurant Simulation\n");
   printf(" Please choose the Test you want to run\n");
   scanf("%d",&testCaseChoice);
   switch(testCaseChoice)
   {
     
	 case 1: for( index = 1; index <= TABLE_COUNT; index++)
				tables[index] = 0; // making all tables not available
			testCase1();
			 break;
	 case 2: testCase2();
			 break;
	 case 3: testCase3();
			 break;
	 case 4: testCase4();
	         break;
	 case 5: testCase5();
	         break;
	 case 6: testCase6();
	         break;
	 case 7: testCase7();
			 break;
	 case 8: testCase8();
			 break;
	 case 9: CarlsJr();
			 break;
	 default: printf(" Please enter the choice from the above list\n");
	 
	}
	}
}
// --------------------------------------------------
// t1_t1() -- test1 thread 1
//     This is the rightful lock owner
// --------------------------------------------------
void t1_t1() {
    t1_l1.Acquire();
    t1_s1.V();  // Allow t1_t2 to try to Acquire Lock
 
    printf ("%s: Acquired Lock %s, waiting for t3\n",currentThread->getName(),
	    t1_l1.getName());
    t1_s3.P();
    printf ("%s: working in CS\n",currentThread->getName());
    for (int i = 0; i < 1000000; i++) ;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
	    t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

// --------------------------------------------------
// t1_t2() -- test1 thread 2
//     This thread will wait on the held lock.
// --------------------------------------------------
void t1_t2() {

    t1_s1.P();	// Wait until t1 has the lock
    t1_s2.V();  // Let t3 try to acquire the lock

    printf("%s: trying to acquire lock %s\n",currentThread->getName(),
	    t1_l1.getName());
    t1_l1.Acquire();

    printf ("%s: Acquired Lock %s, working in CS\n",currentThread->getName(),
	    t1_l1.getName());
    for (int i = 0; i < 10; i++)
	;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
	    t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

// --------------------------------------------------
// t1_t3() -- test1 thread 3
//     This thread will try to release the lock illegally
// --------------------------------------------------
void t1_t3() {

    t1_s2.P();	// Wait until t2 is ready to try to acquire the lock

    t1_s3.V();	// Let t1 do it's stuff
    for ( int i = 0; i < 3; i++ ) {
	printf("%s: Trying to release Lock %s\n",currentThread->getName(),
	       t1_l1.getName());
	t1_l1.Release();
    }
}

// --------------------------------------------------
// Test 2 - see TestSuite() for details
// --------------------------------------------------
Lock t2_l1("t2_l1");		// For mutual exclusion
Condition t2_c1("t2_c1");	// The condition variable to test
Semaphore t2_s1("t2_s1",0);	// To ensure the Signal comes before the wait
Semaphore t2_done("t2_done",0);     // So that TestSuite knows when Test 2 is
                                  // done

// --------------------------------------------------
// t2_t1() -- test 2 thread 1
//     This thread will signal a variable with nothing waiting
// --------------------------------------------------
void t2_t1() {
    t2_l1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	   t2_l1.getName(), t2_c1.getName());
    t2_c1.Signal(&t2_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t2_l1.getName());
    t2_l1.Release();
    t2_s1.V();	// release t2_t2
    t2_done.V();
}

// --------------------------------------------------
// t2_t2() -- test 2 thread 2
//     This thread will wait on a pre-signalled variable
// --------------------------------------------------
void t2_t2() {
    t2_s1.P();	// Wait for t2_t1 to be done with the lock
    t2_l1.Acquire();
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t2_l1.getName(), t2_c1.getName());
    t2_c1.Wait(&t2_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t2_l1.getName());
    t2_l1.Release();
}
// --------------------------------------------------
// Test 3 - see TestSuite() for details
// --------------------------------------------------
Lock t3_l1("t3_l1");		// For mutual exclusion
Condition t3_c1("t3_c1");	// The condition variable to test
Semaphore t3_s1("t3_s1",0);	// To ensure the Signal comes before the wait
Semaphore t3_done("t3_done",0); // So that TestSuite knows when Test 3 is
                                // done

// --------------------------------------------------
// t3_waiter()
//     These threads will wait on the t3_c1 condition variable.  Only
//     one t3_waiter will be released
// --------------------------------------------------
void t3_waiter() {
    t3_l1.Acquire();
    t3_s1.V();		// Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t3_l1.getName(), t3_c1.getName());
    t3_c1.Wait(&t3_l1);
    printf("%s: freed from %s\n",currentThread->getName(), t3_c1.getName());
    t3_l1.Release();
    t3_done.V();
}


// --------------------------------------------------
// t3_signaller()
//     This threads will signal the t3_c1 condition variable.  Only
//     one t3_signaller will be released
// --------------------------------------------------
void t3_signaller() {

    // Don't signal until someone's waiting
    
    for ( int i = 0; i < 5 ; i++ ) 
	t3_s1.P();
    t3_l1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	   t3_l1.getName(), t3_c1.getName());
    t3_c1.Signal(&t3_l1);
    printf("%s: Releasing %s\n",currentThread->getName(), t3_l1.getName());
    t3_l1.Release();
    t3_done.V();
}
 
// --------------------------------------------------
// Test 4 - see TestSuite() for details
// --------------------------------------------------
Lock t4_l1("t4_l1");		// For mutual exclusion
Condition t4_c1("t4_c1");	// The condition variable to test
Semaphore t4_s1("t4_s1",0);	// To ensure the Signal comes before the wait
Semaphore t4_done("t4_done",0); // So that TestSuite knows when Test 4 is
                                // done

// --------------------------------------------------
// t4_waiter()
//     These threads will wait on the t4_c1 condition variable.  All
//     t4_waiters will be released
// --------------------------------------------------
void t4_waiter() {
    t4_l1.Acquire();
    t4_s1.V();		// Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t4_l1.getName(), t4_c1.getName());
    t4_c1.Wait(&t4_l1);
    printf("%s: freed from %s\n",currentThread->getName(), t4_c1.getName());
    t4_l1.Release();
    t4_done.V();
}


// --------------------------------------------------
// t2_signaller()
//     This thread will broadcast to the t4_c1 condition variable.
//     All t4_waiters will be released
// --------------------------------------------------
void t4_signaller() {

    // Don't broadcast until someone's waiting
    
    for ( int i = 0; i < 5 ; i++ ) 
	t4_s1.P();
    t4_l1.Acquire();
    printf("%s: Lock %s acquired, broadcasting %s\n",currentThread->getName(),
	   t4_l1.getName(), t4_c1.getName());
    t4_c1.Broadcast(&t4_l1);
    printf("%s: Releasing %s\n",currentThread->getName(), t4_l1.getName());
    t4_l1.Release();
    t4_done.V();
}
// --------------------------------------------------
// Test 5 - see TestSuite() for details
// --------------------------------------------------
Lock t5_l1("t5_l1");		// For mutual exclusion
Lock t5_l2("t5_l2");		// Second lock for the bad behavior
Condition t5_c1("t5_c1");	// The condition variable to test
Semaphore t5_s1("t5_s1",0);	// To make sure t5_t2 acquires the lock after
                                // t5_t1

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a condition under t5_l1
// --------------------------------------------------
void t5_t1() {
    t5_l1.Acquire();
    t5_s1.V();	// release t5_t2
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t5_l1.getName(), t5_c1.getName());
    t5_c1.Wait(&t5_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t5_l1.getName());
    t5_l1.Release();
}

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a t5_c1 condition under t5_l2, which is
//     a Fatal error
// --------------------------------------------------
void t5_t2() {
    t5_s1.P();	// Wait for t5_t1 to get into the monitor
    t5_l1.Acquire();
    t5_l2.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	   t5_l2.getName(), t5_c1.getName());
    t5_c1.Signal(&t5_l2);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t5_l2.getName());
    t5_l2.Release();
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t5_l1.getName());
    t5_l1.Release();
}


// --------------------------------------------------
// TestSuite()
//     This is the main thread of the test suite.  It runs the
//     following tests:
//
//       1.  Show that a thread trying to release a lock it does not
//       hold does not work
//
//       2.  Show that Signals are not stored -- a Signal with no
//       thread waiting is ignored
//
//       3.  Show that Signal only wakes 1 thread
//
//	 4.  Show that Broadcast wakes all waiting threads
//
//       5.  Show that Signalling a thread waiting under one lock
//       while holding another is a Fatal error
//
//     Fatal errors terminate the thread in question.
// --------------------------------------------------

void TestSuite()
{
 Thread *t;
    char *name;
    int i;
	int choice;
    printf(" MENU\n");
	printf(" 1... Test 1 \n 2...Test 2 \n 3... Test 3 \n 4... Test 4\n 5... Test 5 \n");
	printf("Enter Choice\n");
	scanf("%d",&choice);
    // Test 1
	switch(choice)
    {	
	case 1: printf("Starting Test 1\n");
			t = new Thread("t1_t1");
			t->Fork((VoidFunctionPtr)t1_t1,0);
			t = new Thread("t1_t2");
			t->Fork((VoidFunctionPtr)t1_t2,0);
			t = new Thread("t1_t3");
			t->Fork((VoidFunctionPtr)t1_t3,0);
			// Wait for Test 1 to complete
			for (  i = 0; i < 2; i++ )
			t1_done.P();
			break;
    // Test 2
case 2:    printf("Starting Test 2.  Note that it is an error if thread t2_t2\n");
		   printf("completes\n");

		   t = new Thread("t2_t1");
		   t->Fork((VoidFunctionPtr)t2_t1,0);

		   t = new Thread("t2_t2");
		   t->Fork((VoidFunctionPtr)t2_t2,0);

			// Wait for Test 2 to complete
			t2_done.P();
			break;
    // Test 3

    case 3: printf("Starting Test 3\n");

			for (  i = 0 ; i < 5 ; i++ ) {
			name = new char [20];
			sprintf(name,"t3_waiter%d",i);
			t = new Thread(name);
		t->Fork((VoidFunctionPtr)t3_waiter,0);
		}
		t = new Thread("t3_signaller");
		t->Fork((VoidFunctionPtr)t3_signaller,0);

		// Wait for Test 3 to complete
		for (  i = 0; i < 2; i++ )
		t3_done.P();
		break;
    // Test 4
	case 4:
			printf("Starting Test 4\n");

			for (  i = 0 ; i < 5 ; i++ ) {
			name = new char [20];
			sprintf(name,"t4_waiter%d",i);
			t = new Thread(name);
			t->Fork((VoidFunctionPtr)t4_waiter,0);
		}
			t = new Thread("t4_signaller");
			t->Fork((VoidFunctionPtr)t4_signaller,0);

			// Wait for Test 4 to complete
			for (  i = 0; i < 6; i++ )
			t4_done.P();
			break;
    // Test 5
	case 5:
			printf("Starting Test 5.  Note that it is an error if thread t5_t1\n");
			printf("completes\n");

			t = new Thread("t5_t1");
			t->Fork((VoidFunctionPtr)t5_t1,0);

			t = new Thread("t5_t2");
			t->Fork((VoidFunctionPtr)t5_t2,0);
			break;
}
}

