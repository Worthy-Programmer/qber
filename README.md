  ### Developer(s): 
  EE24B016, EE24B047, EE24B122,  ChatGPT (for comments and guidance)
  
  ### Date: 
  29th September 2024
  
  ### Purpose: 
  Analyze Photon Detector Timestamps to Estimate BER and Visibility with Guard Bands
  
  ### Description:
  This program reads photon detection timestamps from a CSV file, bins the data into a 32ns time window,
    and identifies a 3ns window where the total count of timestamps is maximized. It divides the 3ns window
    into three 1ns bins (C1, D1, C2) to estimate the bit error rate (BER1) and visibility (V1).
    The program then applies 100ps guard bands between the 1ns bins, discards timestamps within the guard bands,
    and recalculates the counts to estimate a new BER (BER2) and visibility (V2).

  ### Input(s):
  - A CSV file containing photon detector timestamps in the first column (in picoseconds).
      The second column (if present) is ignored.

  ### Output(s):
  - Group, BER1, Visibility1, BER2, and Visibility2 printed to the console.
  - (Optional) Output to a file for future analysis.

 ### Key Operations:
  - Modulo operation to bin timestamps within a 32ns window.
  - Sliding window algorithm to identify the 3ns window with the maximum count.
  - Guard bands applied between consecutive 1ns bins to discard erroneous data.

  ### Usage:
  - Compile and run the program by providing a CSV file as input:
      Example:
      ./a.out timestamps.csv
  - CSV format:
    
      timestamp1, value1
    
      timestamp2, value2
    
      (Only the first column is used in the analysis)
