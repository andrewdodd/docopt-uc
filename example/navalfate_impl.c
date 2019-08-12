#include "navalfate_autogen.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
  MAX_SHIPS = 5,
  MAX_NAME_LEN = 50,
};

static uint8_t shipCount = 0;
static char shipNames[MAX_SHIPS][MAX_NAME_LEN] = {0};

static void printShip(int idx) { printf("[%u]: %s\r\n", idx, shipNames[idx]); }

char const *Navalfate_handle_Help(DocoptArgs *args) {
  return Navalfate_getHelpText();
}

char const *Navalfate_handle_Error(DocoptArgs *args) {
  printf("Hmm...we had an error\r\n");
  return NULL;
}

char const *Navalfate_handle_Ships(DocoptArgs *args) {
  if (args->help) {
    printf("Print the list of ships\r\n");
    return NULL;
  }

  printf("Ships: %u\r\n", shipCount);
  for (int i = 0; i < shipCount; i++) {
    printShip(i);
  }
  return NULL;
}

char const *Navalfate_handle_ShipCreate(DocoptArgs *args) {
  if (args->help) {
    printf("Create a ship\r\n");
    return NULL;
  }

  if (args->namedCount > 0) {
    return "no named args used";
  }

  if (args->posCount > 1) {
    return "too many arguments";
  }

  if (args->posCount == 0) {
    return "too few arguments";
  }

  if (shipCount == MAX_SHIPS) {
    return "too many ships already";
  }

  int len = strlen(args->posValue[0]);
  if (len > 50) {
    return "try a shorter name";
  }

  strcpy(shipNames[shipCount], args->posValue[0]);
  shipCount++;
  printShip(shipCount - 1);

  return NULL;
}

char const *Navalfate_handle_ShipMove(DocoptArgs *args) {
  if (args->posCount != 3) {
    return "incorrect args count";
  }

  int idx = -1;
  for (int i = 0; i < shipCount; i++) {
    if (strcmp(args->posValue[0], shipNames[i]) == 0) {
      idx = i;
      break;
    }
  }
  if (idx == -1) {
    return "no ship with matching name";
  }

  printf("Moving ship %s to %s, %s", args->posValue[0], args->posValue[1],
         args->posValue[2]);
  if (args->namedCount == 0) {
    printf(" at unknown speed\r\n");
  } else if (strcmp("speed", args->namedLabel[0]) == 0) {
    printf(" at %s knots\r\n", args->namedValue[0]);
  } else {
    printf(" with an unsupported option list:\r\n");
    for (int i = 0; i < args->namedCount; i++) {
      printf("  > %s = %s\r\n", args->namedLabel[i], args->namedValue[i]);
    }
  }

  return NULL;
}
char const *Navalfate_handle_ShipShoot(DocoptArgs *args) {

  return "Not implemented";
}

char const *Navalfate_handle_MineSet(DocoptArgs *args) {
  return "Not implemented";
}

char const *Navalfate_handle_MineRemove(DocoptArgs *args) {
  return "Not implemented";
}

char const *Navalfate_handle_MineSweep(DocoptArgs *args) {
  return "Not implemented";
}
