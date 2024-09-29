#include <stdio.h>
#include <stdlib.h>
#include <math.h> // For fmod

#define WINDOW_SIZE 32000 // 32ns in picoseconds
#define GUARD_BAND 100    // 100ps guard band
#define GROUP "M"

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

// Function to find max sum window and calculate BER1 and Visibility1
double find_max_sum_window(int histogram[], int size, int window_size, double *BER1, double *V1)
{
  int max_sum = 0;
  int current_sum = 0;
  int start_index = 0;

  if (window_size > size)
  {
    return -1;
  }

  // Calculate the sum of the first window
  for (int i = 0; i < window_size; i++)
  {
    current_sum += histogram[i];
  }

  max_sum = current_sum;

  // Slide the window and update the sum
  for (int i = window_size; i < size; i++)
  {
    current_sum += histogram[i] - histogram[i - window_size];
    if (current_sum > max_sum)
    {
      max_sum = current_sum;
      start_index = i - window_size + 1;
    }
  }

  // Divide the window into 3 equal parts (without guard bands)
  int part_size = window_size / 3;
  int C1 = 0, D1 = 0, C2 = 0;

  for (int i = start_index; i < start_index + window_size; i++)
  {
    if (i < start_index + part_size)
    {
      C1 += histogram[i];
    }
    else if (i < start_index + 2 * part_size)
    {
      D1 += histogram[i];
    }
    else
    {
      C2 += histogram[i];
    }
  }

  // Calculate BER and visibility
  *BER1 = 1 - (double)D1 / (C1 + D1 + C2);
  *V1 = (double)D1 / (C1 + C2);

  return start_index;
}

// Function to apply guard bands and calculate BER2 and Visibility2
void apply_guard_bands_and_calculate(int histogram[], int start_index, int window_size, double *BER2, double *V2)
{
  int part_size = window_size / 3;
  int C1 = 0, D1 = 0, C2 = 0;

  // Apply guard bands (ignore the first and last 100ps of each 1ns bin)
  for (int i = start_index; i < start_index + window_size; i++)
  {
    // If within the guard band, skip the timestamp
    if ((i % 1000) < GUARD_BAND || (i % 1000) > (1000 - GUARD_BAND))
    {
      continue; // Skip timestamps in the guard band
    }

    // Otherwise, accumulate counts
    if (i < start_index + part_size)
    {
      C1 += histogram[i];
    }
    else if (i < start_index + 2 * part_size)
    {
      D1 += histogram[i];
    }
    else
    {
      C2 += histogram[i];
    }
  }

  // Calculate BER and visibility after applying guard bands
  *BER2 = 1 - (double)D1 / (C1 + D1 + C2);
  *V2 = (double)D1 / (C1 + C2);
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    printf("Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  const char *filename = argv[1]; // Input CSV file name (timestamps in picoseconds)
  int histogram[WINDOW_SIZE];

  // Process the CSV file and populate the histogram
  process_csv_and_create_histogram(filename, histogram);

  double BER1, V1, BER2, V2;

  // Find the 3ns window with the maximum sum and calculate BER1 and Visibility1
  int start_index = find_max_sum_window(histogram, WINDOW_SIZE, 3000, &BER1, &V1);

  // Apply guard bands and calculate BER2 and Visibility2
  apply_guard_bands_and_calculate(histogram, start_index, 3000, &BER2, &V2);

  // Output the results
  printf("%s,%lf,%lf,%lf,%lf\n", GROUP, BER1, V1, BER2, V2);
  return 0;
}
