import { Component, OnInit, ChangeDetectionStrategy, ViewChild, ElementRef } from '@angular/core';
import { ImageService } from '../core/image.service';

export interface Vector2D { // TODO: Move reusable interface into root
  x: number;
  y: number;
}

@Component({
  selector: 'app-single-pic',
  templateUrl: './single-pic.component.html',
  styleUrls: ['./single-pic.component.scss'],
})
export class SinglePicComponent implements OnInit {
  @ViewChild('editCanvas', { static: true }) canvas: ElementRef<HTMLCanvasElement>;
  descriptionText = 'Upload New Picture';
  screenSize = { // TODO: Move into global config singleton
    x: 800,
    y: 480,
  };
  sourceUrl: string | ArrayBuffer = null;
  previewUrl: string = null;
  preprocessedImage: Uint8ClampedArray;
  cdfArray: number[];
  byteArray: Uint8ClampedArray;
  isDragging = false;
  isPreview = false;
  adjustExposure = 0;
  adjustContrast = 0;
  adjustEqualize = 1;

  constructor(
    private imageService: ImageService,
    ) {}

  ngOnInit(): void {
  }

  onFileSelect(selectFileEvent: any) {
    if (selectFileEvent.target.files && selectFileEvent.target.files[0]) {
      this.onNewImageFile(selectFileEvent.target.files[0]);
    }
  }

  onFileDrop(imageEvent: any) {
    const imageFile = imageEvent.dataTransfer.files[0];
    if (!imageFile.type.match(/image.*/)) {
      return;
    }
    this.onNewImageFile(imageFile);
  }

  onTransmit(): void {
    this.imageService.uploadSingleImage(this.byteArray).subscribe(() => {

    });
  }

  private onNewImageFile(file: any): void {
    const reader = new FileReader();
    reader.onload = (event) => {
      this.preprocessImage(event.target.result); // Process image on invisible canvas
    },
      reader.readAsDataURL(file);
  }

  /**
   * Preprocesses an image & obtains a smaller image to be further processed.
   * Uses global variables.
   * @param rawImageUrl Source ImageUrl to be preprocessed
   */
  private preprocessImage(rawImageUrl: string | ArrayBuffer): void{
    const image = new Image();
    image.onload = () => {
      const size: Vector2D = { // Size of input image
        x: image.width,
        y: image.height,
      };
      const target: Vector2D = { // Target size (of this project's display)
        x: this.screenSize.x,
        y: this.screenSize.y,
      };
      const clipped: Vector2D = this.coverCrop(size, target); // Crop image to aspect ratio
      const offset = {  // Put image in middle
        x: (size.x - clipped.x) / 2,
        y: (size.y - clipped.y) / 2,
      };
      const ctx = this.canvas.nativeElement.getContext('2d') as CanvasRenderingContext2D;
      this.canvas.nativeElement.width = target.x;
      this.canvas.nativeElement.height = target.y;
      ctx.drawImage(
        image,              // img    - Image to draw on canvas
        offset.x,           // sx     - Offset position, if aspect ratio not fitting
        offset.y,           // sy
        clipped.x,          // swidth - Width of cropped selection on original
        clipped.y,          // sheight
        0,                  // x      - Draw position
        0,                  // y
        target.x,  // width  - Drawing width on canvas
        target.y,  // heigt
      );
      // Store cropped & resized color image for preview (of color original)
      this.sourceUrl = this.canvas.nativeElement.toDataURL();
      // Extract image data to further process
      const imageData = ctx.getImageData(0, 0, target.x, target.y);
      const color = imageData.data;
      const bw = this.rgbaToGreyscale(color);
      // Store all one time preprocessed data globally, can be accessed by image adjusting functions
      this.preprocessedImage = bw;
      this.cdfArray = this.cdf(this.histogram(bw)); // get histogram distribution
      const bwRgb = this.greyscaleToRbga(bw);
      const bwData = new ImageData(bwRgb, this.screenSize.x, this.screenSize.y);
      // ctx is further used every time if image to url conversion is needed
      ctx.putImageData(bwData, 0, 0);
      this.render(); // Does dithering & image processing
    };
    image.src = rawImageUrl as string;
  }


/**
 * Processes the image data: Brightness, contrast, equalization.
 * Called once on image load and each time the image adjustment sliders are changed
 */
  render(): void {
      const bw = this.processImage(this.preprocessedImage); // Equalizes the histogram (with mix factor)
      this.bwToImageUrl(bw);
  }

  /**
   * Converts a black and white array to image url previewUrl, stored globally.
   * @param bw Black and white Unit8ClampedArray
   */
  private bwToImageUrl(bw: Uint8ClampedArray) {
    this.byteArray = this.dither(bw);
    const ditheredRgb = this.greyscaleToRbga(this.byteArray);
    const ditheredImage = new ImageData(ditheredRgb, this.screenSize.x, this.screenSize.y);
    const ctx = this.canvas.nativeElement.getContext('2d') as CanvasRenderingContext2D;
    ctx.putImageData(ditheredImage, 0, 0);
    this.previewUrl = this.canvas.nativeElement.toDataURL();
  }

  /**
   * Processes the image regarding brightness, equalization, contrast.
   * Equalization has a wet/dry-like mix factor.
   * @param bwInput Black and white Unit8ClampedArray
   */
  private processImage(bwInput: Uint8ClampedArray): Uint8ClampedArray {
    /*
    Instead of using brightness processing (adding a brightness value) which always
    influences the contrast, an exposure-like processing is chosen.
    */
    const exposure = Math.pow(2, this.adjustExposure);
    const equalized = bwInput.map(
      (brightness) => {
        // User can mix between 100% equalized image and original with adjustEqualize slider.
        const cont = this.adjustContrast * brightness - 128 * this.adjustContrast;
        return Math.floor(((255 * this.cdfArray[brightness]) * this.adjustEqualize + brightness * (1 - this.adjustEqualize)) * exposure + cont);
      }
    );
    return equalized;
  }

  /**
   * Calculates the histogram of a greyscale image array.
   * @param img 8 bit black and white image array
   */
  private histogram(img: Uint8ClampedArray): number[] {
    const pixels = img.length;
    let histogram: number[] = new Array(256).fill(0);
    img.forEach(
      (element) => {
        histogram[element] = histogram[element] + 1;
    });
    // Normalize histogram
    histogram = histogram.map(
      (histogramValue) => {
        return histogramValue / pixels;
      }
    );
    return histogram;
  }

  /**
   * Cumulative distribution, integral of histogram
   * @param histogram Normalized histogram
   */
  private cdf(histogram: number[]): number[] {
    const cdf: number[] = new Array(256).fill(0);
    let sum = 0;
    histogram.forEach(
      (histogramValue, index) => {
        sum = sum + histogramValue;
        cdf[index] = sum;
      }
    );
    return cdf;
  }

  /**
   * Greyscale needs to be weighted from color values.
   * See https://en.wikipedia.org/wiki/Grayscale for details.
   * @param rgbInput RGBA Array
   */
  private rgbaToGreyscale(rgbInput: Uint8ClampedArray): Uint8ClampedArray {
    let greyScaleImageBytes = new Uint8ClampedArray(Math.floor(rgbInput.length / 4)).fill(0);
    greyScaleImageBytes = greyScaleImageBytes.map(
      (value, index) => {
        return 0.2126 * rgbInput[index * 4] + 0.7152 * rgbInput[index * 4 + 1] + 0.0722 * rgbInput[index * 4 + 2];
      }
    );
    return greyScaleImageBytes;
  }

  private greyscaleToRbga(bwInput: Uint8ClampedArray): Uint8ClampedArray {
    const color: number[] = [];
    for (const elem of bwInput) {
      color.push(elem);
      color.push(elem);
      color.push(elem);
      color.push(255);
    }
    return new Uint8ClampedArray(color);
  }

  private coverCrop(size: Vector2D, target: Vector2D): Vector2D {
    // Handle aspect ratio of input like a object-fit: cover
    const targetRatio = target.x / target.y;
    if (size.x / size.y > targetRatio) {
      // Wider than target image
      return ({
        x: size.y * (targetRatio),
        y: size.y,
      });
    } else {
      // Taller than target image (or fitting)
      return ({
        x: size.x,
        y: size.x / (targetRatio),
      });
    }
  }

  private dither(image: Uint8ClampedArray): Uint8ClampedArray {
    const width = this.screenSize.x;
    const height = this.screenSize.y;
    const dithered = image;
    for (let i = 0; i < height; i++) {
      for (let j = 0; j < width; j++) {
        const ci = i * width + j;
        const cc = dithered[ci];
        const rc = (cc < 128 ? 0 : 255);
        const err = cc - rc;
        dithered[ci] = rc;
        if (j + 1 < width) {

          dithered[ci + 1] += (err * 7) / 16;
        }
        if (i + 1 === height) {
          continue;
        }
        if (j > 0) {
          dithered[ci + width - 1] += (err * 3) / 16;
        }
        dithered[ci + width] += (err * 5) / 16;
        if (j + 1 < width) {
          dithered[ci + width + 1] += (err * 1) / 16;
        }
      }
    }
    return dithered;
  }

}
