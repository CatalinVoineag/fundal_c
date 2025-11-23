#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include<unistd.h>

typedef struct {
  int size;
  int capacity;
  char **elements;
  char *error;
} pictures_t;

void add_picture(pictures_t *pictures, char *value, int index) {
  if (pictures->size == pictures->capacity) {
    pictures->capacity *= 2;
    pictures->elements = realloc(pictures->elements, sizeof(char *) * pictures->capacity);
  }
  if (pictures->elements == NULL) { return; }

  pictures->elements[index] = malloc(strlen(value) + 1); 
  strcpy(pictures->elements[index], value);
  pictures->size++;
}

pictures_t preloaded_pictures() {
  char buffer[128];
  FILE *fp = popen("hyprctl hyprpaper listloaded", "r");
  pictures_t preloaded_pictures = { .size = 0, .capacity = 1};
  preloaded_pictures.elements = malloc(sizeof(char *) * preloaded_pictures.capacity);

  if (fp == NULL) {
      perror("popen failed");
      return preloaded_pictures;
  }

  int index = 0;

  while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    char *error = strstr(buffer, "Couldn't connect to");

    if (error == NULL) {
      buffer[strcspn(buffer, "\n")] = 0; // strip \n
      add_picture(&preloaded_pictures, buffer, index);
      index++;
    } else {
      preloaded_pictures.error = "hyprpaper is not running";
    }
  }

  pclose(fp);
  return preloaded_pictures;
}

bool picture_preloaded(pictures_t *preloaded, char *picture) {
  for (int i=0; i < preloaded->size; i++) {
    if (strcmp(preloaded->elements[i], picture) == 0) {
      return true;
    }
  } 
  return false;
}

void change_wallpaper(pictures_t *pictures, char *pictures_path, pictures_t *preloaded) {
  srand(time(NULL));
  char change_command[128];
  int random_index = rand() % pictures->size;

  while (picture_preloaded(preloaded, pictures->elements[random_index]) == true) {
    random_index = rand() % pictures->size;
  }

  snprintf(
    change_command,
    sizeof(change_command),
    "hyprctl hyprpaper reload ',%s%s'",
    pictures_path,
    pictures->elements[random_index]
  );

  system(change_command);
}

void free_pictures_memory(pictures_t *pictures) {
  for (int i = 0; i < pictures->size; i++) {
    free(pictures->elements[i]);
  }

  free(pictures->elements);
}

int handle_help(int argc, char **argv) {
  if (argc > 1) {
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-help") == 0) {
      printf("Usage: fundal [options...]\n");
      printf("fundal -- changes the wallpaper to a random one \n");
      printf("fundal 30 -- changes the wallpaper now and every 30 minutes \n");
      return 1;
    }
  }
  
  return 0;
}

int main(int argc, char **argv) {
  char buffer[128];
  char pictures_path[] = "~/Pictures/wallpapers/";
  pictures_t preloaded = preloaded_pictures();
  int minutes_argv; 

  if (handle_help(argc, argv) == 1) {
    return 1;
  };

  if (argc > 1) {
    minutes_argv = atoi(argv[1]);

    if (minutes_argv <= 0) {
      printf("Pass a positive number if you want to set a timer \n");
      return 1;
    }
  }

  if (preloaded.error != NULL) {
    printf("%s \n", preloaded.error);
    free_pictures_memory(&preloaded);
    return 1;
  }

  FILE *fp = popen("ls ~/Pictures/wallpapers/", "r");

  if (fp == NULL) {
      perror("popen failed");
      return 1;
  }

  pictures_t pictures = { .size = 0, .capacity = 10};
  pictures.elements = malloc(sizeof(char *) * pictures.capacity);
  int index = 0;

  while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    buffer[strcspn(buffer, "\n")] = 0; // strip \n
    add_picture(&pictures, buffer, index);
    index++;
  }

  pclose(fp);

  while (true) {
    change_wallpaper(&pictures, pictures_path, &preloaded);
    sleep(60 * minutes_argv);
  }

  free_pictures_memory(&pictures);
  return 0;
}
