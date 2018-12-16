//Christopher McLaughlin
#include <iostream>
#include <thread>
#include <algorithm>
#include <mutex>
#include <random>
#include <condition_variable>
using namespace std;

int buffer[4] = { 0, 0, 0, 0 };
int maxBuffer[4] = { 6, 5, 4, 3 };
int seed = 1;
int activeProduct = 0;
int activePart = 0;
int iterationsNoChange = 0;
mutex m1;
condition_variable_any waitProduct, waitPart;

//Common Functions
bool checkIfEmpty(int *modified);
bool checkIfSame(int *modified, int*old);
void deadlock(int j);
//Part Worker Functions
void partInfo(int *placeBuffer, int i, int j);
void partUpdated(int *placeBuffer);
void placeRequest(int(&placeBuffer)[4]);
void placeParts(int *placeBuffer, int i, int j);
void placeUpdate(int *placeBuffer);
void PartWorker(int i);
//Product Worker Functions
void productInfo(int *pickBuffer, int i, int j);
void productUpdated(int *pickBuffer);
void pickRequest(int(&pickBuffer)[4]);
void pickParts(int pickBuffer[4], int i, int j);
void pickUpdate(int *pickBuffer);
void ProductWorker(int i);


/* Common Functions */
bool checkIfEmpty(int *modified) {
	if (modified[0] == 0
		&& modified[1] == 0
		&& modified[2] == 0
		&& modified[3] == 0) {
		return true;
	}
	else {
		return false;
	}
}

bool checkIfSame(int *modified, int*old) {
	if (modified[0] == old[0]
		&& modified[1] == old[1]
		&& modified[2] == old[2]
		&& modified[3] == old[3]) {
		return true;
	}
	else {
		return false;
	}
}

void deadlock(int j) {
	cout << "Deadlock Detected" << endl;
	cout << "Aborted Iteration: " << j << endl;
	cout << endl;
}

/* Part Worker Functions */
void partInfo(int *placeBuffer, int i, int j) {
	cout << "Part Worker ID: " << i << endl;
	cout << "Iteration: " << j << endl;
	cout << "Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
	cout << "Place Request: (" << placeBuffer[0] << "," << placeBuffer[1] << "," << placeBuffer[2] << "," << placeBuffer[3] << ")" << endl;
}

void partUpdated(int *placeBuffer) {
	cout << "Updated Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
	cout << "Updated Place Request: (" << placeBuffer[0] << "," << placeBuffer[1] << "," << placeBuffer[2] << "," << placeBuffer[3] << ")" << endl;
}

void placeRequest(int(&placeBuffer)[4]) {
	int arr[4][4] = { { 0, 0, 0, 4 },
					  { 0, 0, 1, 3 },
					  { 0, 0, 2, 2 },
					  { 0, 1, 1, 2 } };

	srand(seed++);
	int r = rand() % 4;
	srand(seed++);
	random_shuffle(&arr[r][0], &arr[r][4]);

	for (int i = 0; i < 4; i++) {
		placeBuffer[i] = arr[r][i];
	}
}

void placeParts(int *placeBuffer, int i, int j) {
	m1.lock();
	partInfo(placeBuffer, i, j);

	int old[4];
	for (int k = 0; k < 4; k++) {
		old[k] = placeBuffer[k];
	}

	placeUpdate(placeBuffer);
	bool complete = false;
	while (!complete) {
		if (checkIfEmpty(placeBuffer)) {
			partUpdated(placeBuffer);
			cout << endl;
			iterationsNoChange = 0;
			complete = true;
		}
		else if (checkIfSame(placeBuffer, old)) {
			partUpdated(placeBuffer);
			iterationsNoChange++;
			if (activeProduct == 0) {
				iterationsNoChange = 0;
				deadlock(j);
				complete = true;
			}
			else if (iterationsNoChange == 50) {
				iterationsNoChange = 0;
				deadlock(j);
				complete = true;
			}
			else {
				cout << endl;
				waitProduct.notify_all();
				waitPart.wait(m1);
				partInfo(placeBuffer, i, j);
				placeUpdate(placeBuffer);
			}
		}
		else {
			if (activeProduct == 0) {
				partUpdated(placeBuffer);
				deadlock(j);
				complete = true;
			}
			else {
				partUpdated(placeBuffer);
				cout << endl;
				iterationsNoChange = 0;

				waitProduct.notify_all();
				waitPart.wait(m1);

				partInfo(placeBuffer, i, j);
				for (int k = 0; k < 4; k++) {
					old[k] = placeBuffer[k];
				}

				placeUpdate(placeBuffer);
			}
		}
	}
	if (activeProduct == 0) {
		m1.unlock();
		waitPart.notify_all();
	}
	else {
		m1.unlock();
		waitProduct.notify_all();
	}
}

void placeUpdate(int *placeBuffer) {
	for (int i = 0; i < 4; i++) {
		if (maxBuffer[i] < placeBuffer[i] + buffer[i]) {
			placeBuffer[i] = (placeBuffer[i] + buffer[i]) - maxBuffer[i];
			buffer[i] = maxBuffer[i];
		}
		else {
			buffer[i] = buffer[i] + placeBuffer[i];
			placeBuffer[i] = 0;
		}
	}
}

void PartWorker(int i) {
	m1.lock();
	activePart++;
	m1.unlock();
	for (int j = 1; j <= 5; j++) {
		int placeBuffer[4];
		placeRequest(placeBuffer);
		placeParts(placeBuffer, i, j);
	}
	m1.lock();
	activePart--;
	waitProduct.notify_all();
	m1.unlock();
	return;
}

/* Product Worker Functions */
void productInfo(int *pickBuffer, int i, int j) {
	cout << "Product Worker ID: " << i << endl;
	cout << "Iteration: " << j << endl;
	cout << "Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
	cout << "Pickup Request: (" << pickBuffer[0] << "," << pickBuffer[1] << "," << pickBuffer[2] << "," << pickBuffer[3] << ")" << endl;
}

void productUpdated(int *pickBuffer) {
	cout << "Updated Buffer State: (" << buffer[0] << "," << buffer[1] << "," << buffer[2] << "," << buffer[3] << ")" << endl;
	cout << "Updated Pickup Request: (" << pickBuffer[0] << "," << pickBuffer[1] << "," << pickBuffer[2] << "," << pickBuffer[3] << ")" << endl;
}

void pickRequest(int(&pickBuffer)[4]) {
	int arr[2][4] = { { 0, 1, 1, 3 },
					  { 0, 1, 2, 2 } };

	srand(seed++);
	int r = rand() % 2;

	srand(seed++);
	random_shuffle(&arr[r][0], &arr[r][4]);

	for (int i = 0; i < 4; i++) {
		pickBuffer[i] = arr[r][i];
	}
}

void pickParts(int pickBuffer[4], int i, int j) {
	m1.lock();
	productInfo(pickBuffer, i, j);

	int old[4];
	for (int k = 0; k < 4; k++) {
		old[k] = pickBuffer[k];
	}

	pickUpdate(pickBuffer);
	bool complete = false;
	while (!complete) {
		if (checkIfEmpty(pickBuffer)) {
			productUpdated(pickBuffer);
			cout << endl;
			iterationsNoChange = 0;
			complete = true;
		}
		else if (checkIfSame(pickBuffer, old)) {
			productUpdated(pickBuffer);
			iterationsNoChange++;
			if (activePart == 0) {
				iterationsNoChange = 0;
				deadlock(j);
				complete = true;
			}
			else if (iterationsNoChange == 50) {
				iterationsNoChange = 0;
				deadlock(j);
				complete = true;
			}
			else {
				cout << endl;
				waitPart.notify_all();
				waitProduct.wait(m1);
				productInfo(pickBuffer, i, j);

				pickUpdate(pickBuffer);
			}
		}
		else {
			if (activePart == 0) {
				productUpdated(pickBuffer);
				deadlock(j);
				complete = true;
			}
			else {
				productUpdated(pickBuffer);
				cout << endl;
				iterationsNoChange = 0;

				waitPart.notify_all();
				waitProduct.wait(m1);

				productInfo(pickBuffer, i, j);
				for (int k = 0; k < 4; k++) {
					old[k] = pickBuffer[k];
				}

				pickUpdate(pickBuffer);
			}
		}
	}
	if (activePart == 0) {
		m1.unlock();
		waitProduct.notify_all();
	}
	else {
		m1.unlock();
		waitPart.notify_all();
	}
}

void pickUpdate(int *pickBuffer) {
	for (int i = 0; i < 4; i++) {
		if (buffer[i] >= pickBuffer[i]) {
			buffer[i] = buffer[i] - pickBuffer[i];
			pickBuffer[i] = 0;
		}
		else {
			pickBuffer[i] = pickBuffer[i] - buffer[i];
			buffer[i] = 0;
		}
	}
}

void ProductWorker(int i) {
	m1.lock();
	activeProduct++;
	m1.unlock();
	for (int j = 1; j <= 5; j++) {
		int pickBuffer[4];
		pickRequest(pickBuffer);
		pickParts(pickBuffer, i, j);
	}
	m1.lock();
	activeProduct--;
	waitPart.notify_all();
	m1.unlock();

	return;
}

int main() {
	const int m = 20, n = 16; //m, number of part workers
							  //n, number of product workers
							  //m > n
	thread partW[m];
	thread prodW[n];

	/* Create Product And Part Workers */
	for (int i = 0; i < n; i++) {
		partW[i] = thread(PartWorker, i);
		prodW[i] = thread(ProductWorker, i);
	}
	/* Finish Creating Part Workers */
	for (int i = n; i < m; i++) {
		partW[i] = thread(PartWorker, i);
	}
	/* Join Product and Part Workers */
	for (int i = 0; i < n; i++) {
		partW[i].join();
		prodW[i].join();
	}
	/* Finish Joining Part Workers */
	for (int i = n; i < m; i++) {
		partW[i].join();
	}

	cout << "Finish!" << endl;
	getchar();
	getchar();
	return 0;
}
