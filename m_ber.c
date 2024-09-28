#include <stdio.h>
#include <stdlib.h>
#include <math.h> // For fmod

#define WINDOW_SIZE 32000 // 32ns in picoseconds

// Function to read timestamps from CSV, modulo them by 32000ps, and populate histogram
void process_csv_and_create_histogram(const char *filename, int *histogram)
{
  FILE *file = fopen(filename, "r");
  if (!file)
  {
    printf("Error: Could not open file %s\n", filename);
    exit(1);
  }

  // Skip the first row (header)
  char buffer[1024];
  fgets(buffer, sizeof(buffer), file); // Ignoring the first row (header)

  double timestamp;
  double second_column_value; // To read and discard the second column

  // Initialize histogram with 0
  for (int i = 0; i < WINDOW_SIZE; i++)
  {
    histogram[i] = 0;
  }

  // Read timestamps from the first column of the CSV and ignore the second column
  while (fscanf(file, "%lf,%lf", &timestamp, &second_column_value) == 2)
  {
    // Apply modulo-32000 to fit within 32ns window
    int mod_timestamp = (int)(timestamp) % WINDOW_SIZE;

    // Increment the corresponding bin in the histogram
    histogram[mod_timestamp]++;
  }

  // Close the file
  fclose(file);
}

int main()
{
  const char *filename = "timestamps_1.csv"; // Input CSV file name (timestamps in picoseconds)
  int histogram[WINDOW_SIZE];

  // Process the CSV file and populate the histogram
  process_csv_and_create_histogram(filename, histogram);

  // For testing: print the non-zero histogram values (bins with counts)
  printf("Histogram of timestamps within 32ns window (mod 32000ps):\n");
  for (int i = 0; i < WINDOW_SIZE; i++)
  {
    if (histogram[i] > 0)
    {
      printf("Bin %d: %d counts\n", i, histogram[i]);
    }
  }

  return 0;
}
