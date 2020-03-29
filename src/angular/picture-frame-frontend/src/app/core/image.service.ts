import { Injectable } from '@angular/core';
import { HttpClient, HttpHeaders } from '@angular/common/http';

@Injectable({
  providedIn: 'root'
})
export class ImageService {
  API_KEY = 'API_KEY';
  apiURL = '/api';

  constructor(private httpClient: HttpClient) { }

  public putOnScreen(clamped: Uint8ClampedArray) {
    return this.httpClient.post(`${this.apiURL}/image/on-screen`, this.byteToByte(clamped));
  }

  private byteToByte(clampedArray: Uint8ClampedArray): number[] {
    const boolArray: boolean[] = [];
    // tslint:disable-next-line: prefer-for-of
    for (let i = 0; i < clampedArray.length; i++) { // Nasty Uint8ClampedArray
      boolArray.push(clampedArray[i] === 0);
    }
    const byteArray: number[] = [];
    for (let i = 0; i < boolArray.length / 8; i++) {
      let byte = 0;
      for (let j = 0; j < 8; j++) {
        // tslint:disable-next-line: no-bitwise
        byte <<= 1;
        if (boolArray[8 * i + j]) {
          // tslint:disable-next-line: no-bitwise
          byte |= 1;
        }
      }
      byteArray.push(byte);
    }
    return byteArray;
  }

}

