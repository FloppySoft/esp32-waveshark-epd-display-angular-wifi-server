

import { Injectable } from '@angular/core';
import { HttpClient, HttpHeaders } from '@angular/common/http';

@Injectable({
  providedIn: 'root'
})
export class SystemService {
  API_KEY = 'API_KEY';
  apiURL = '/api';

  constructor(private httpClient: HttpClient) { }

  public getHeap() {
    return this.httpClient.get(`${this.apiURL}/system/heap`);
  }

}
