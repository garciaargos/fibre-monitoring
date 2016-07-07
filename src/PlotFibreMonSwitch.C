/*
 * PlotFibreMonSwitch.C(time_plot)
 * 
 * Use:
 * 
 *  - root -b -q PlotFibreMonSwitch.C(number, width, height)
 * 
 *   number is the last hours to plot, all data if 0.
 *   width and height are the size of the canvases in pixels.
 * 
 * Plots the attenuation of the fibres, transmitted power, received power 
 * and temperatures of the SFPs from the log file.
 * 
 * Uses a fibre mapping file.
 * 
 * Compensates for attenuators (in the fibre map file).
 * 
 * Carlos García Argos (carlos.garcia.argos@cern.ch), 2014
 * 
 * Changelog:
 *  18/06/2014: first version. Plot attenuation, Ptx, Prx and temperature
 *  30/06/2014: change to work with different interval sizes (in minutes)
 *  01/07/2014: count time passed from beginning to end by looking at the 
 *              dates/times, so the plots are good even if some points 
 *              were lost.
 *  10/07/2014: special "do_time_plot" value -1 prints also 12 and 24 hours
 *              together with all data. Improves performance in the Raspberry
 *              Pi, avoiding processing the log file 3 times.
 *  14/08/2014: fixed weird timeshift at the beginning of the plots.
 */
#include <time.h>

#define FIBRES 4
#define INTERVAL 10

/*
 * Extract year from the date in a string.
 */

int getYear(const char *date) {
  int y;
  char temp[20];
  
  temp[0] = date[0];
  temp[1] = date[1];
  temp[2] = date[2];
  temp[3] = date[3];
  temp[4] = '\0';
  
  y = atoi(temp);
  
  return y;
}

/*
 * Extract month from the date in a string.
 */

int getMonth(const char *date) {
  int y;
  char temp[20];
  
  temp[0] = date[5];
  temp[1] = date[6];
  temp[2] = '\0';
  
  y = atoi(temp);
  
  return y;
}

/*
 * Extract day of the month from the date in a string.
 */
int getDay(const char *date) {
  int y;
  char temp[20];
  
  temp[0] = date[8];
  temp[1] = date[9];
  temp[2] = '\0';
  
  y = atoi(temp);
  
  return y;
}

/*
 * Extract hours from the time in a string.
 */
int getHour(const char *time) {
  int y;
  char temp[20];
  
  temp[0] = time[0];
  temp[1] = time[1];
  temp[2] = '\0';
  
  y = atoi(temp);
  
  return y;
}

/*
 * Extract minutes from the time in a string.
 */
int getMinute(const char *time) {
  int y;
  char temp[20];
  
  temp[0] = time[3];
  temp[1] = time[4];
  temp[2] = '\0';
  
  y = atoi(temp);
  
  return y;
}

/*
 * Get the timestamp (Unix Epoch) for date, time in
 * strings, as seen in the log file.
 */
int getTimestamp(const char *date, const char *time) {
  int ts;
  struct tm epoc, current;
  
  epoc.tm_hour = 0; epoc.tm_min = 0; epoc.tm_sec = 0;
  epoc.tm_year = 1970-1900; epoc.tm_mon = 0; epoc.tm_mday = 1;
  epoc.tm_isdst = -1;   // Daylight Saving Time correction: let mktime() guess
  
  current.tm_hour = getHour(time); current.tm_min = getMinute(time); current.tm_sec = 0;
  current.tm_year = getYear(date)-1900; current.tm_mon = getMonth(date)-1; current.tm_mday = getDay(date);
  current.tm_isdst = -1;

  ts = difftime(mktime(&current), mktime(&epoc));
  
  return ts;
}

/*
 * Extract the index of the transmitter from the fibre map
 */
int getFibreIndexTx(TTree *t, char *txswitch) {
  int index = -1;
  int len = t->GetEntries();
  char fromSw[100];
  TBranch *br = t->GetBranch("fromSw");
  
  for (int i = 0; i < len; i++) {
    br->GetEntry(i);
    cout << fromSw <<endl;
    if (strstr(fromSw, txswitch) != NULL) {
      cout << "Found one\n";
      index = i;
      i = len;
    }
  }
  
  return index;
}

/*
 * In principle, plot over the last "time_plot" hours (0 to N).
 * If time_plot < 0, do 3 plots: all data, 12 hours and 24 hours. For CPU efficiency in the Raspberry Pi
 */
void PlotFibreMonSwitch(int do_time_plot = 0, int width = 1400, int height = 900) { // Plot over the last "time_plot" hours
  int time_plot_array[3] = {0, 24, 12};
  float fontsize = 0.045;
  const char filename[200] = "MergedLog.txt";
  const char file_fibremap[200] = "fibremap.txt";
  int nfibres = FIBRES;
  float **fibres_values_temp;  // Ptx, Prx, Attenuation, Attenuator, Temperatures
  char **fibres_names_temp;
  int time_plot = 0;    // Avoid breaking things
  if (do_time_plot > 0) {
    time_plot = do_time_plot;
  }
  
  char title[FIBRES][20];
  
  TTree *data;
  TTree *fibremap;
  TTree *fibres_values = new TTree("Fibres values", "Fibres values");

  fibres_values_temp = (float**)calloc(nfibres, sizeof(float*));
  fibres_names_temp = (char**)calloc(nfibres, sizeof(char*));
  for (int j = 0; j < nfibres; j++) {
    fibres_values_temp[j] = (float*)calloc(5, sizeof(float));
    fibres_names_temp[j] = (char*)calloc(100, sizeof(char));
  }

  // Data from the fibremap file
  char fromSw[100], toSw[100], fibrename[100], fibrename_val[100];
  int fibre, fromPort, toPort, fibre_idx;
  float attenuator;
  
  fibremap = new TTree("Fibre mapping", "Fibre mapping");
  fibremap->ReadFile(file_fibremap, "fibre/I:fromSw/C:fromPort/I:toSw/C:toPort/I:fibrename/C:attenuator/F");
  fibremap->SetBranchAddress("fibre", &fibre);
  fibremap->SetBranchAddress("fromSw", fromSw);
  fibremap->SetBranchAddress("fromPort", &fromPort);
  fibremap->SetBranchAddress("toSw", toSw);
  fibremap->SetBranchAddress("toPort", &toPort);
  fibremap->SetBranchAddress("fibrename", fibrename);
  fibremap->SetBranchAddress("attenuator", &attenuator);
  
  nfibres = fibremap->GetEntries();
  

  // Data for the destination TTree containing each fibre attenuation (and the rest)
  float x, att, Ptx, Prx ,Ptx_fibreval, Prx_fibreval, temperature_val;
  char titletemp[50];
  TBranch *branch_fibre = fibres_values->Branch("fibre_idx", &fibre_idx, "fibre_idx/I");
  TBranch *branch_fibrename = fibres_values->Branch("fibrename_val", fibrename_val, "fibrename_val/C");
  TBranch *branch_time = fibres_values->Branch("x", &x, "x/F");
  TBranch *branch_att = fibres_values->Branch("Ptx_fibreval", &Ptx_fibreval, "Ptx_fibreval/F");
  TBranch *branch_temperature = fibres_values->Branch("temperature_val", &temperature_val, "temperature_val/F");
  TBranch *branch_Ptx = fibres_values->Branch("Prx_fibreval", &Prx_fibreval, "Prx_fibreval/F");
  TBranch *branch_Prx = fibres_values->Branch("Attenuation", &att, "Att/F");
  
  cout << "Got " << nfibres << " fibres mapped" << endl;
  
  // Load logging data
  data = new TTree("Fibre data", "Fibre data");
  char date[200], month[20], day[20], time[20], hostname[100], t2[20];
  float temp, voltage, current, Ptx, Prx;
  data->ReadFile(filename, "month/C:day:time:hostname:date:t2:temp/F:voltage:current:Ptx:Prx");
  data->SetBranchAddress("date", date);
  data->SetBranchAddress("month", month);
  data->SetBranchAddress("day", day);
  data->SetBranchAddress("time", time);
  data->SetBranchAddress("hostname", hostname);
  data->SetBranchAddress("t2", t2);
  data->SetBranchAddress("temp", &temp);
  data->SetBranchAddress("voltage", &voltage);
  data->SetBranchAddress("current", &current);
  data->SetBranchAddress("Ptx", &Ptx);
  data->SetBranchAddress("Prx", &Prx);
  
  int datalen = data->GetEntries();
  int time_entries = datalen/nfibres;
  
  // Get timestamps for beginning and end of log
  data->GetEntry(0);
  int time0 = getTimestamp(date, time);
  data->GetEntry(datalen-1);
  int timeN = getTimestamp(date, time);
  
  time_entries = (timeN - time0)/60/INTERVAL + 1;
  
  // Process data and do mapping
  int pos = 0;
  int line_time = 0;
  int thisfibre = 0;
  for (int pos = 0; pos < datalen; pos++) {
    data->GetEntry(pos);
    if (line_time != (getTimestamp(date, time) - time0)/60/INTERVAL) {
      thisfibre = 0;
      for (int k = 0; k < nfibres; k++) {
        fibres_values_temp[k][2] = fibres_values_temp[k][0] - fibres_values_temp[k][1] - fibres_values_temp[k][3];
        
        x = (float)line_time*INTERVAL*60; // *INTERVAL*60;
        
        strcpy(fibrename_val, fibres_names_temp[k]);
        Ptx_fibreval = fibres_values_temp[k][0];
        Prx_fibreval = fibres_values_temp[k][1];
        temperature_val = fibres_values_temp[k][4];
        att = fibres_values_temp[k][2];
        fibre_idx = k;
        // Save TTree element
        fibres_values->Fill();
      }
      line_time = (getTimestamp(date, time) - time0)/60/INTERVAL;
    }
    char hostname_switch[100];
    int port_switch = -1;
    char *tok;
    
    tok = strtok(hostname, ":");
    if (tok != NULL) {
      strcpy(hostname_switch, tok);
      tok = strtok(NULL, ":");
      if (tok != NULL) {
        port_switch = atoi(tok);
      }
    }
    
    // Find hostname and port in the fibre maps 
    int idx_tx = -1;
    int idx_rx = -1;
    for (int k = 0; k < nfibres; k++) {
      fibremap->GetEntry(k);
      if ((strstr(hostname, fromSw) != NULL) && ((port_switch == -1) || (port_switch == fromPort))) {
        idx_tx = k;
        strcpy(fibres_names_temp[k], fibrename);
        fibres_values_temp[idx_tx][3] = attenuator;
      }
      if ((strstr(hostname, toSw) != NULL) && ((port_switch == -1) || (port_switch == toPort))) {
        idx_rx = k;
      }
    }
    fibres_values_temp[idx_tx][0] = Ptx;
    fibres_values_temp[idx_tx][4] = temp;
    fibres_values_temp[idx_rx][1] = Prx;
    
    thisfibre++;
  }

  // Plot
  TGraph **Attenuation;
  TGraph **TxPower, **RxPower, **Temperature;
  TLegend *att_legend = new TLegend(0.85, 0.85, 0.99, 0.99);
  TLegend *temperature_legend = new TLegend(0.85, 0.85, 0.99, 0.99);
  TLegend *txpower_legend = new TLegend(0.85, 0.85, 0.99, 0.99);
  TLegend *rxpower_legend = new TLegend(0.85, 0.85, 0.99, 0.99);
  float **y_val, **x_val, **txpower_val, **rxpower_val, max_att, min_att, max_txpower, min_txpower, max_rxpower, min_rxpower, **temp_val, max_temp, min_temp;
  char **fibrenames;
  
  min_att = 1e9;
  max_att = -1;
  min_rxpower = 1e9;
  max_rxpower = -1e9;
  min_txpower = 1e9;
  max_txpower = -1e9;
  min_temp = 1e9;
  max_temp = -1e9;

  y_val = (float**)calloc(nfibres, sizeof(float*));
  x_val = (float**)calloc(nfibres, sizeof(float*));
  temp_val = (float**)calloc(nfibres, sizeof(float*));
  txpower_val = (float**)calloc(nfibres, sizeof(float*));
  rxpower_val = (float**)calloc(nfibres, sizeof(float*));
  fibrenames = (char**)calloc(nfibres, sizeof(char*));
  for (int i = 0; i < nfibres; i++) {
    y_val[i] = (float*)calloc(time_entries, sizeof(float));
    x_val[i] = (float*)calloc(time_entries, sizeof(float));
    for (int j = 0; j < time_entries; j++) {
      x_val[i][j] = -1;
    }
    temp_val[i] = (float*)calloc(time_entries, sizeof(float));
    txpower_val[i] = (float*)calloc(time_entries, sizeof(float));
    rxpower_val[i] = (float*)calloc(time_entries, sizeof(float));
    fibrenames[i] = (char*)calloc(time_entries, sizeof(char));
  }

  Attenuation = (TGraph**)calloc(nfibres, sizeof(TGraph*));
  Temperature = (TGraph**)calloc(nfibres, sizeof(TGraph*));
  TxPower = (TGraph**)calloc(nfibres, sizeof(TGraph*));
  RxPower = (TGraph**)calloc(nfibres, sizeof(TGraph*));
  int nentries = fibres_values->GetEntries();
  // Get data from TTree into the arrays
  for (int k = 0; k < nentries; k++) {
    fibres_values->GetEntry(k);
    x_val[(int)fibre_idx][(int)x/INTERVAL/60] = x + INTERVAL*60;
    y_val[(int)fibre_idx][(int)x/INTERVAL/60] = att;
    temp_val[(int)fibre_idx][(int)x/INTERVAL/60] = temperature_val;
    txpower_val[(int)fibre_idx][(int)x/INTERVAL/60] = Ptx_fibreval;
    rxpower_val[(int)fibre_idx][(int)x/INTERVAL/60] = Prx_fibreval;
    strcpy(fibrenames[fibre_idx], fibrename_val);
  }
  /* 
   * Create all TGraphs and TLegends
   */
  for (int i = 0; i < nfibres; i++) {
    // Fix null values
    for (int j = 0; j < time_entries; j++) {
      if (x_val[i][j] < 0) {
        x_val[i][j] = x_val[i][j - 1] + INTERVAL*60;
        y_val[i][j] = y_val[i][j - 1];
        temp_val[i][j] = temp_val[i][j - 1];
        txpower_val[i][j] = txpower_val[i][j - 1];
        rxpower_val[i][j] = rxpower_val[i][j - 1];
      }
      // If using time_plot, zoom in only in the interesting interval
      if ((time_plot == 0) || (j + time_plot*60/INTERVAL > time_entries)) {
        if (max_att < y_val[i][j])
          max_att = y_val[i][j];
        if (min_att > y_val[i][j])
          min_att = y_val[i][j];
        if (max_txpower < txpower_val[i][j])
          max_txpower = txpower_val[i][j];
        if (min_txpower > txpower_val[i][j])
          min_txpower = txpower_val[i][j];
        if (max_rxpower < rxpower_val[i][j])
          max_rxpower = rxpower_val[i][j];
        if (min_rxpower > rxpower_val[i][j])
          min_rxpower = rxpower_val[i][j];
        if (max_temp < temp_val[i][j])
          max_temp = temp_val[i][j];
        if (min_temp > temp_val[i][j])
          min_temp = temp_val[i][j];
      }
    }
    Attenuation[i] = new TGraph(time_entries, x_val[i], y_val[i]);
    Temperature[i] = new TGraph(time_entries, x_val[i], temp_val[i]);
    TxPower[i] = new TGraph(time_entries, x_val[i], txpower_val[i]);
    RxPower[i] = new TGraph(time_entries, x_val[i], rxpower_val[i]);
    att_legend->AddEntry(Attenuation[i], fibrenames[i], "LP");
    temperature_legend->AddEntry(Attenuation[i], fibrenames[i], "LP");
    txpower_legend->AddEntry(Attenuation[i], fibrenames[i], "LP");
    rxpower_legend->AddEntry(Attenuation[i], fibrenames[i], "LP");
  }

  // X axis reference for zooming in the last N hours intervals
  int rangeinit = (time_entries*INTERVAL - time_plot*60)*60;
  int rangeend = INTERVAL*60*(time_entries + 5*time_plot/12);   // Variable spacing depending on the number of hours, to make room for the legend

  // Time reference for the plots X axis
  data->GetEntry(0);
  char date1[20], date2[20];
  char time1[20], time2[20];
  sprintf(date1, date);
  sprintf(time1, time);
  data->GetEntry(datalen - 1);
  sprintf(date2, date);
  sprintf(time2, time);
  TDatime T0(getYear(date1), getMonth(date1), getDay(date1), getHour(time1), getMinute(time1) - INTERVAL, 00);
  int X0 = T0.Convert();
  gStyle->SetTimeOffset(X0);
  TDatime T1(getYear(date1), getMonth(date1), getDay(date1), getHour(time1), getMinute(time1), 00);
  int X1 = T1.Convert()-X0;
  TDatime T2(getYear(date2), getMonth(date2), getDay(date2), getHour(time2), getMinute(time2), 00);
  int X2 = T2.Convert()-X0 + INTERVAL*60;       // Move the lines to the left to leave some space for the legend
  
  /*
   * Attenuation plot
   */
  TCanvas *att_can = new TCanvas("C_att", "Attenuation", width, height);

  for (int i = 0; i < nfibres; i++) {
    Attenuation[i]->Draw(i==0?"AL":"L,same");
    Attenuation[i]->SetLineColor(i + 1);
    Attenuation[i]->SetLineWidth(2);
  }
  Attenuation[0]->GetYaxis()->SetRangeUser(floor(min_att*.95), ceil(max_att*1.05));
  if (time_plot > 0) {
    Attenuation[0]->GetXaxis()->SetRangeUser(rangeinit, rangeend);
  }
  Attenuation[0]->GetYaxis()->SetTitle("Attenuation [dB]");
  Attenuation[0]->GetXaxis()->SetTitle("");
  Attenuation[0]->SetTitle("Fibres attenuation");
  Attenuation[0]->GetXaxis()->SetTimeFormat("#splitline{%d/%m}{%H:%M}");
  Attenuation[0]->GetXaxis()->SetTimeDisplay(1);
  Attenuation[0]->GetXaxis()->SetLabelOffset(0.03);
  
  att_legend->Draw();
  
  // Output to file
  char filename[200];
  if (time_plot > 0) {
    sprintf(filename, "attenuation_%d_hours.png", time_plot);
  }
  else {
    sprintf(filename, "attenuation.png");
  }
  att_can->Print(filename);
  
  if (do_time_plot < 0) {
    for (int i = 1; i < 3; i++) {
      sprintf(filename, "attenuation_%d_hours.png", time_plot_array[i]);
      
      int rangeinit = (time_entries*INTERVAL - time_plot_array[i]*60)*60;
      int rangeend = INTERVAL*60*(time_entries + 5*time_plot_array[i]/12);
      Attenuation[0]->GetXaxis()->SetRangeUser(rangeinit, rangeend);
      float thisattmax = -1.;
      float thisattmin = 1000.;
      for (int j = 0; j < time_entries; j++) {
        for (int k = 0; k < nfibres; k++) {
          if (j + time_plot_array[i]*60/INTERVAL > time_entries) {
            if (y_val[k][j] > thisattmax)
              thisattmax = y_val[k][j];
            if (y_val[k][j] < thisattmin)
              thisattmin = y_val[k][j];
          }
        }
      }
      Attenuation[0]->GetYaxis()->SetRangeUser(floor(thisattmin*.95), ceil(thisattmax*1.05));
      
      att_can->Print(filename);
    }
  }
  
  /*
   * Transmitted power plot
   */
  TCanvas *txpower_can = new TCanvas("TxPower", "TxPower", width, height);
  
  for (int i = 0; i < nfibres; i++) {
    TxPower[i]->Draw(i==0?"AL":"L,same");
    TxPower[i]->SetLineColor(i + 1);
    TxPower[i]->SetLineWidth(2);
  }
  TxPower[0]->GetYaxis()->SetRangeUser((ceil(min_txpower) - 1), (floor(max_txpower) + 1));
  if (time_plot > 0) {
    TxPower[0]->GetXaxis()->SetRangeUser(rangeinit, rangeend); 
  }
  TxPower[0]->GetYaxis()->SetTitle("TxPower [dBm]");
  TxPower[0]->GetXaxis()->SetTitle("");
  TxPower[0]->SetTitle("SFP Transmitted Power");
  TxPower[0]->GetXaxis()->SetTimeFormat("#splitline{%d/%m}{%H:%M}");
  TxPower[0]->GetXaxis()->SetTimeDisplay(1);
  TxPower[0]->GetXaxis()->SetLabelOffset(0.03);
  txpower_legend->Draw();
  
  if (time_plot > 0) {
    sprintf(filename, "txpower_%d_hours.png", time_plot);
  }
  else {
    sprintf(filename, "txpower.png");
  }
  txpower_can->Print(filename);
  if (do_time_plot < 0) {
    for (int i = 1; i < 3; i++) {
      sprintf(filename, "txpower_%d_hours.png", time_plot_array[i]);
      
      int rangeinit = (time_entries*INTERVAL - time_plot_array[i]*60)*60;
      int rangeend = INTERVAL*60*(time_entries + 5*time_plot_array[i]/12);
      TxPower[0]->GetXaxis()->SetRangeUser(rangeinit, rangeend);
      float thistxpmax = -1000.;
      float thistxpmin = 1000.;
      for (int j = 0; j < time_entries; j++) {
        for (int k = 0; k < nfibres; k++) {
          if (j + time_plot_array[i]*60/INTERVAL > time_entries) {
            if (txpower_val[k][j] > thistxpmax)
              thistxpmax = txpower_val[k][j];
            if (txpower_val[k][j] < thistxpmin)
              thistxpmin = txpower_val[k][j];
          }
        }
      }
      TxPower[0]->GetYaxis()->SetRangeUser((ceil(thistxpmin) - 1), (floor(thistxpmax) + 1));
      
      txpower_can->Print(filename);
    }
  }
  
  
  
  /*
   * Received power plot
   */
  TCanvas *rxpower_can = new TCanvas("RxPower", "RxPower", width, height);
  
  for (int i = 0; i < nfibres; i++) {
    RxPower[i]->Draw(i==0?"AL":"L,same");
    RxPower[i]->SetLineColor(i + 1);
    RxPower[i]->SetLineWidth(2);
  }
  RxPower[0]->GetYaxis()->SetRangeUser((ceil(min_rxpower) - 1), (floor(max_rxpower) + 1));
  if (time_plot > 0) {
    RxPower[0]->GetXaxis()->SetRangeUser(rangeinit, rangeend);
  }
  RxPower[0]->GetYaxis()->SetTitle("RxPower [dBm]");
  RxPower[0]->GetXaxis()->SetTitle("");
  RxPower[0]->SetTitle("SFP Received Power");
  RxPower[0]->GetXaxis()->SetTimeFormat("#splitline{%d/%m}{%H:%M}");
  RxPower[0]->GetXaxis()->SetTimeDisplay(1);
  RxPower[0]->GetXaxis()->SetLabelOffset(0.03);
  rxpower_legend->Draw();
  
  if (time_plot > 0) {
    sprintf(filename, "rxpower_%d_hours.png", time_plot);
  }
  else {
    sprintf(filename, "rxpower.png");
  }
  rxpower_can->Print(filename);
  if (do_time_plot < 0) {
    for (int i = 1; i < 3; i++) {
      sprintf(filename, "rxpower_%d_hours.png", time_plot_array[i]);
      
      int rangeinit = (time_entries*INTERVAL - time_plot_array[i]*60)*60;
      int rangeend = INTERVAL*60*(time_entries + 5*time_plot_array[i]/12);
      RxPower[0]->GetXaxis()->SetRangeUser(rangeinit, rangeend);
      float thisrxpmax = -1000.;
      float thisrxpmin = 1000.;
      for (int j = 0; j < time_entries; j++) {
        for (int k = 0; k < nfibres; k++) {
          if (j + time_plot_array[i]*60/INTERVAL > time_entries) {
            if (rxpower_val[k][j] > thisrxpmax)
              thisrxpmax = rxpower_val[k][j];
            if (rxpower_val[k][j] < thisrxpmin)
              thisrxpmin = rxpower_val[k][j];
          }
        }
      }
      
      RxPower[0]->GetYaxis()->SetRangeUser((ceil(thisrxpmin) - 1), (floor(thisrxpmax) + 1));
      
      rxpower_can->Print(filename);
    }
  }
  
   
  /*
   * Temperature plot
   */
  TCanvas *temperature_can = new TCanvas("Temperature", "Temperature", width, height);
  
  for (int i = 0; i < nfibres; i++) {
    Temperature[i]->Draw(i==0?"AL":"L,same");
    Temperature[i]->SetLineColor(i + 1);
    Temperature[i]->SetLineWidth(2);
  }
  Temperature[0]->GetYaxis()->SetRangeUser((ceil(min_temp) - 1), (floor(max_temp) + 1));
  if (time_plot > 0) {
    Temperature[0]->GetXaxis()->SetRangeUser(rangeinit, rangeend);
  }
  Temperature[0]->GetYaxis()->SetTitle("Temperature [^{o}C]");
  Temperature[0]->GetXaxis()->SetTitle("");
  Temperature[0]->SetTitle("SFP Temperature");
  Temperature[0]->GetXaxis()->SetTimeFormat("#splitline{%d/%m}{%H:%M}");
  Temperature[0]->GetXaxis()->SetTimeDisplay(1);
  Temperature[0]->GetXaxis()->SetLabelOffset(0.03);
  temperature_legend->Draw();
  
  if (time_plot > 0) {
    sprintf(filename, "temperature_%d_hours.png", time_plot);
  }
  else {
    sprintf(filename, "temperature.png");
  }
  temperature_can->Print(filename);
  if (do_time_plot < 0) {
    for (int i = 1; i < 3; i++) {
      sprintf(filename, "temperature_%d_hours.png", time_plot_array[i]);
      
      int rangeinit = (time_entries*INTERVAL - time_plot_array[i]*60)*60;
      int rangeend = INTERVAL*60*(time_entries + 5*time_plot_array[i]/12);
      Temperature[0]->GetXaxis()->SetRangeUser(rangeinit, rangeend);
      float thistempmax = -1.;
      float thistempmin = 1000.;
      for (int j = 0; j < time_entries; j++) {
        for (int k = 0; k < nfibres; k++) {
          if (j + time_plot_array[i]*60/INTERVAL > time_entries) {
            if (temp_val[k][j] > thistempmax)
              thistempmax = temp_val[k][j];
            if (temp_val[k][j] < thistempmin)
              thistempmin = temp_val[k][j];
          }
        }
      }
      
      Temperature[0]->GetYaxis()->SetRangeUser((ceil(thistempmin) - 1), (floor(thistempmax) + 1));
      
      temperature_can->Print(filename);
    }
  }
  

  // Free Willy
  free(fibres_values_temp);
  free(fibres_names_temp);
  
}
