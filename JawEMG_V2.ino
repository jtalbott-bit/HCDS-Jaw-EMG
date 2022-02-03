#define CALSAMPLES 1000
#define N 255
#define LONG 500
#define SHORT 25
#define M 1

#define STOP 0
#define GO 1
#define LEFT 2
#define RIGHT 3
#define REVERSE 4
#define TO_IDLE 5
#define NO_CMD 6

void calibrate();

//global variables
int filterArray[N] = {0};
int prevAvg = 0;
long int timeCheck;
long int timeStartHigh = 0;
long int timeHigh = 0;
int out = 0;
double offset = 0;
int threshold = 125;
int cmd = NO_CMD;
int cmdArray[M + 1] = {0, 0};
int cmdNumber = 0;
int STOPtime = 900;
long int timeSincePrevCmd;
int cmdResetTime = 2000;
bool cmdComplete = false;
bool signalToLow = false;


void setup()
{
  //set baud rate
  Serial.begin(9600);

  //find offset of input data
  long int sum = 0;
  for (int i = 0; i < CALSAMPLES; ++i) {
    sum += analogRead(A0);
  }
  offset = sum / CALSAMPLES;

  //  calibrate();
}

void loop()
{
  //local variables
  long int sum = 0;

  //read in data from A0
  int val = analogRead(A0) - offset;
  if (val < 0) val = -1 * val;

  //updates moving average filter and sum values
  for (int i = 0; i < N - 1; ++i) {
    filterArray[i + 1] = filterArray[i];
    sum += filterArray[i];
  }
  //set first value in array as the newly read data
  filterArray[0] = val;
  //add the new value to the sum
  sum += val;
  //find the moving average
  int avg = sum / N;

  //check to see if the data has passed the threshold
  //sets the inital time high and the time to check the output status
  if (prevAvg < threshold && avg > threshold) {
    if (out == false) {
      timeStartHigh = millis();
    }
    out = true;
    timeCheck = millis() + 150;
  }

  //check to if the most recent values recorded are still an active signal
  sum = 0;
  for (int i = 0; i < 100; ++i) {
    if (filterArray[i] > 0.5 * threshold) ++sum;
  }
  if (sum >= 2)signalToLow = false;
  else signalToLow = true;

  //if there hasn't been a threshold passage in 0.25 seconds set output low
  if (millis() > timeCheck && signalToLow) {
    if (out == true) {
      timeHigh = millis() - timeStartHigh;
      timeStartHigh = 0;
    }
    out = false;
  }

  //if out is high for more than STOPtime, the cmd is to stop
  if (millis() - timeStartHigh > STOPtime && out) {
    cmd  = STOP;
    timeHigh = 0;
    cmdComplete = false;
    cmdNumber = 0;
  }
  else if (millis() - timeSincePrevCmd < cmdResetTime) {
    //check to see if the incoming bit was a long bit
    if (timeHigh >= LONG && !cmdComplete) {
      cmdArray[cmdNumber] = LONG;
      timeHigh = 0;
      ++cmdNumber;
      if (cmdNumber > M) {
        cmdNumber = 0;
        cmdComplete = true;
      }
      timeSincePrevCmd = millis();
    }
    else if (timeHigh >= SHORT && !cmdComplete) {
      cmdArray[cmdNumber] = SHORT;
      timeHigh = 0;
      ++cmdNumber;
      if (cmdNumber > M) {
        cmdNumber = 0;
        cmdComplete = true;
      }
      timeSincePrevCmd = millis();
    }
  } else if (!cmdComplete) {
    timeSincePrevCmd = millis();
    cmdNumber = 0;
  }

  //if a cmd has been fully transmitted, determine what cmd was sent
  if (cmdComplete) {
    cmdComplete = false;
    if (cmdArray[0] == LONG) {
      if (cmdArray[1] == LONG) {
        cmd = REVERSE;
      } else if (cmdArray[1] == SHORT) {
        cmd = LEFT;
      }
    } else if (cmdArray[0] == SHORT) {
      if (cmdArray[1] == LONG) {
        cmd = RIGHT;
      } else if (cmdArray[1] == SHORT) {
        cmd = GO;
      }
    }
  }

  //print outputs
//      Serial.print(avg);
  //    Serial.print(' ');
    Serial.print(cmd);
  //  Serial.print(' ');
  //  Serial.print();
    Serial.print(' ');
  Serial.println(10 * out);

  //  for (int i = 0; i < M; ++i) {
  //    Serial.print(cmdArray[i]);
  //  }
  //  Serial.println();
  //set the previous average to the current one for next iteration
  prevAvg = avg;
}

void calibrate() {
  //local variables
  long int sum = 0;
  long int total = 0;
  int avg, val;
  int runningAvg = 0;
  int maximum = 0;

  for (long int i = 0; i < 5000; ++i) {
    sum = 0;
    //read in data from A0
    val = analogRead(A0) - offset;
    if (val < 0) val = -1 * val;

    //updates moving average filter and sum values
    for (int i = 0; i < N - 1; ++i) {
      filterArray[i + 1] = filterArray[i];
      sum += filterArray[i];
    }
    //set first value in array as the newly read data
    filterArray[0] = val;
    //add the new value to the sum
    sum += val;
    //find the moving average
    avg = sum / N;
    if (avg > maximum) {
      maximum = avg;
    }
    total += avg;
    runningAvg = total / (i + 1);

    Serial.print(avg);
    Serial.print('\t');
    Serial.println(runningAvg);

    //delay(1);
  }
  Serial.println("Calibration Done!");
  delay(1000);
}
